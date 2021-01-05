/*
 * Copyright 2017, alex at staticlibs.net
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * File:   running_request.hpp
 * Author: alex
 *
 * Created on February 13, 2017, 9:23 PM
 */

#ifndef STATICLIB_HTTP_RUNNING_REQUEST_HPP
#define STATICLIB_HTTP_RUNNING_REQUEST_HPP

#include <cstdint>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "curl/curl.h"

#include "staticlib/config.hpp"

#include "staticlib/http/request_options.hpp"
#include "staticlib/http/resource_info.hpp"

#include "curl_deleters.hpp"
#include "curl_headers.hpp"
#include "curl_info.hpp"
#include "curl_options.hpp"
#include "running_request_pipe.hpp"
#include "request_ticket.hpp"

namespace staticlib {
namespace http {

class running_request {
    enum class req_state {
        created, receiving_headers, receiving_data, receiving_trailers
    };

    // holds data passed to curl
    std::string url;
    request_options options;
    std::unique_ptr<std::istream> post_data;
    curl_headers headers;
    std::unique_ptr<CURL, curl_easy_deleter> handle;

    // run details
    std::shared_ptr<running_request_pipe> pipe;
    bool paused = false;
    std::string error;
    sl::concurrent::growing_buffer buf;
    req_state state = req_state::created;

public:
    running_request(CURLM* multi_handle, request_ticket&& ticket) :
    url(std::move(ticket.url)),
    options(std::move(ticket.options)),
    post_data(std::move(ticket.post_data)),    
    handle(curl_easy_init(), curl_easy_deleter(multi_handle)),
    pipe(std::move(ticket.pipe)) {        
        if (nullptr == handle.get()) throw http_exception(TRACEMSG("Error initializing cURL handle"));
        CURLMcode errm = curl_multi_add_handle(multi_handle, handle.get());
        if (errm != CURLM_OK) throw http_exception(TRACEMSG(
                "cURL multi_add error: [" + curl_multi_strerror(errm) + "], url: [" + this->url + "]"));
        apply_curl_options(this, this->url, this->options, this->post_data, this->headers, this->handle);
    }

    running_request(const running_request&) = delete;

    running_request& operator=(const running_request&) = delete;

    // finalization must be noexcept anyway,
    // so lets tie finalization to destruction
    ~running_request() STATICLIB_NOEXCEPT {
        auto info = [this] {
            try {
                return curl_collect_info(handle.get());
            } catch (const std::exception& e) {
                append_error(TRACEMSG(e.what()));
                return resource_info();
            }
        }();
        pipe->set_resource_info(std::move(info));
        if (!error.empty()) {
            pipe->append_error(error);
        }
        pipe->shutdown();
    }

    const std::string& get_url() const {
        return url;
    }

    bool is_paused() {
        return paused;
    }

    void unpause() {
        this->paused = false;
        curl_easy_pause(handle.get(), CURLPAUSE_CONT);
    }

    void append_error(const std::string& msg) {
        if (error.empty()) {
            error.append("Error reported for request, url: [" + url + "]\n");
        } else {
            error.append("\n");
        }
        error.append(msg);
    }
    
    request_options& get_options() {
        return options;
    }

    CURL* easy_handle() {
        return handle.get();
    }

    bool data_queue_is_full() {
        return pipe->data_queue_is_full();
    }

    // http://stackoverflow.com/a/9681122/314015
    size_t write_headers(char* buffer, size_t size, size_t nitems) {
        if (req_state::created == state) {
            curl_info ci(handle.get());
            long code = ci.getinfo_long(CURLINFO_RESPONSE_CODE);
            // https://curl.haxx.se/mail/lib-2011-03/0160.html
            if (100 != code) {
                pipe->set_response_code(code);
                if (options.abort_on_response_error && code >= 400) {
                    append_error(TRACEMSG("HTTP response error, status code: [" + sl::support::to_string(code) + "]"));
                    return 0;
                } else {
                    state = req_state::receiving_headers;
                }
            }
        } else if (req_state::receiving_data == state) {
            state = req_state::receiving_trailers;
        }
        size_t len = size*nitems;
        auto opt = curl_parse_header(buffer, len);
        if (opt) {
            pipe->emplace_header(std::move(opt.value()));
        }
        return len;
    }

    size_t write_data(char* buffer, size_t size, size_t nitems) {
        if (req_state::receiving_headers == state) {
            state = req_state::receiving_data;
        } else if (req_state::receiving_data != state) {
            append_error(TRACEMSG("System error: invalid state on 'write_data'"));
            return 0;
        }
        size_t len = size * nitems;
        // chunk is too big for stack
        buf.resize(len);
        std::memcpy(buf.data(), buffer, buf.size());
        bool placed = pipe->write_some_data(std::move(buf));
        if (!placed) {
            paused = true;
            return CURL_WRITEFUNC_PAUSE;
        }
        return len;
    }

    size_t read_data(char* buffer, size_t size, size_t nitems) {
        size_t len = size * nitems;
        auto src = sl::io::streambuf_source(post_data->rdbuf());
        std::streamsize read = sl::io::read_all(src, {buffer, len});
        return static_cast<size_t> (read);
    }
};

} // namespace
}


#endif /* STATICLIB_HTTP_RUNNING_REQUEST_HPP */

