/*
 * Copyright 2016, alex at staticlibs.net
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
 * File:   single_threaded_http_resource.cpp
 * Author: alex
 *
 * Created on March 25, 2017, 10:08 PM
 */

#include "single_threaded_http_resource.hpp"

#include <chrono>
#include <thread>
#include <vector>

#include "curl/curl.h"

#include "staticlib/io.hpp"
#include "staticlib/pimpl/pimpl_forward_macros.hpp"

#include "curl_deleters.hpp"
#include "curl_headers.hpp"
#include "curl_info.hpp"
#include "curl_options.hpp"
#include "curl_utils.hpp"
#include "http_resource_impl.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace io = staticlib::io;
namespace sc = staticlib::config;

using headers_type = const std::vector<std::pair<std::string, std::string>>&;

} // namespace

class single_threaded_http_resource::impl : public http_resource::impl {
    enum class resource_state {
        created,
        writing_headers,
        response_info_filled
    };
    
    std::unique_ptr<CURLM, curl_multi_deleter> multi_handle;
    std::unique_ptr<CURL, curl_easy_deleter> handle;
    
    // holds data passed to curl
    std::string url;
    http_session_options session_options;
    http_request_options options;
    std::unique_ptr<std::istream> post_data;
    curl_headers request_headers;
    
    // run details
    http_resource_info info;
    uint16_t status_code = 0;
    std::vector<std::pair<std::string, std::string>> response_headers;
    std::vector<char> buf;
    size_t buf_idx = 0;
    bool open = false;
    resource_state state = resource_state::created;
    std::string error;
    
public:
    impl(const http_session_options& session_options, const std::string& url,
            std::unique_ptr<std::istream> post_data, http_request_options options) :
    multi_handle(curl_multi_init(), curl_multi_deleter()),
    handle(curl_easy_init(), curl_easy_deleter(this->multi_handle.get())),
    url(url.data(), url.length()),
    session_options(session_options),
    options(std::move(options)),
    post_data(std::move(post_data)) {
        if (nullptr == multi_handle.get()) throw httpclient_exception(TRACEMSG("Error initializing cURL multi handle"));
        apply_curl_multi_options(this->session_options, this->multi_handle);
        CURLMcode errm = curl_multi_add_handle(multi_handle.get(), handle.get());
        if (errm != CURLM_OK) throw httpclient_exception(TRACEMSG(
                "cURL multi_add error: [" + curl_multi_strerror(errm) + "], url: [" + this->url + "]"));
        apply_curl_options(this, this->url, this->options, this->post_data, this->request_headers, this->handle);
        this->open = true;
    }

    std::streamsize read(http_resource&, sc::span<char> span) {
        size_t ulen = span.size();
        fill_buffer();
        if (0 == buf.size()) {
            if (open) {
                return 0;
            } else {
                this->info = curl_collect_info(handle.get());
                return std::char_traits<char>::eof();
            }
        }
        // return from buffer
        size_t avail = buf.size() - buf_idx;
        size_t reslen = avail <= ulen ? avail : ulen;
        std::memcpy(span.data(), buf.data(), reslen);
        buf_idx += reslen;
        return static_cast<std::streamsize> (reslen);
    }

    const std::string& get_url(const http_resource&) const {
        return url;
    }

    uint16_t get_status_code(const http_resource&) const {
        return status_code;
    }

    http_resource_info get_info(const http_resource&) const {
        return info;
    }

    const std::vector<std::pair<std::string, std::string>>& get_headers(const http_resource&) const {
        return response_headers;
    }

    const std::string& get_header(const http_resource&, const std::string& name) const {
        // try cached first
        for (auto& en : response_headers) {
            if (name == en.first) {
                return en.second;
            }
        }
        static std::string empty;
        return empty;
    }

    bool connection_successful(const http_resource& frontend) const {
        return get_status_code(frontend) > 0;
    }
    
    // http://stackoverflow.com/a/9681122/314015
    size_t write_headers(char *buffer, size_t size, size_t nitems) {
        if (resource_state::created == state) {
            curl_info ci(handle.get());
            this->status_code = ci.getinfo_long(CURLINFO_RESPONSE_CODE);
            this->state = resource_state::writing_headers;
        }
        size_t len = size*nitems;
        std::string name{};
        for (size_t i = 0; i < len; i++) {
            if (':' != buffer[i]) {
                name.push_back(buffer[i]);
            } else {
                std::string value{};
                // 2 for ': ', 2 for '\r\n'
                size_t valen = len - i - 2 - 2;
                if (valen > 0) {
                    value.resize(valen);
                    std::memcpy(std::addressof(value.front()), buffer + i + 2, value.length());
                    response_headers.emplace_back(std::move(name), std::move(value));
                    break;
                }
            }
        }
        return len;
    }
    
    size_t write_data(char *buffer, size_t size, size_t nitems) {
        size_t len = size*nitems;
        buf.resize(len);
        std::memcpy(buf.data(), buffer, len);
        return len;
    }

    size_t read_data(char* buffer, size_t size, size_t nitems) {
        std::streamsize len = static_cast<std::streamsize> (size * nitems);
        io::streambuf_source src{post_data->rdbuf()};
        std::streamsize read = io::read_all(src,{buffer, len});
        return static_cast<size_t> (read);
    }

    void append_error(const std::string& msg) {
        if (error.empty()) {
            error.append("Error reported for request, url: [" + url + "]\n");
        } else {
            error.append("\n");
        }
        error.append(msg);
    }

private:
    // curl_multi_socket_action may be used instead of fdset
    void fill_buffer() {
        // some data in buffer
        if (buf_idx < buf.size()) return;
        buf_idx = 0;
        buf.resize(0);
        auto start = std::chrono::system_clock::now();
        // attempt to fill buffer
        while (open && 0 == buf.size()) {
            long timeo = -1;
            CURLMcode err_timeout = curl_multi_timeout(multi_handle.get(), std::addressof(timeo));
            if (err_timeout != CURLM_OK) throw httpclient_exception(TRACEMSG(
                    "cURL timeout error: [" + curl_multi_strerror(err_timeout) + "], url: [" + url + "]"));
            struct timeval timeout = create_timeout_struct(timeo);

            // get file descriptors from the transfers
            fd_set fdread = create_fd();
            fd_set fdwrite = create_fd();
            fd_set fdexcep = create_fd();
            int maxfd = -1;
            CURLMcode err_fdset = curl_multi_fdset(multi_handle.get(), std::addressof(fdread),
                    std::addressof(fdwrite), std::addressof(fdexcep), std::addressof(maxfd));
            if (err_fdset != CURLM_OK) throw httpclient_exception(TRACEMSG(
                    "cURL fdset error: [" + curl_multi_strerror(err_fdset) + "], url: [" + url + "]"));

            // wait or select
            int err_select = 0;
            if (maxfd == -1) {
                std::this_thread::sleep_for(std::chrono::milliseconds{session_options.fdset_timeout_millis});
            } else {
                err_select = select(maxfd + 1, std::addressof(fdread), std::addressof(fdwrite),
                        std::addressof(fdexcep), std::addressof(timeout));
            }

            // do perform if no select error
            if (-1 != err_select) {
                int active = -1;
                CURLMcode err = curl_multi_perform(multi_handle.get(), std::addressof(active));
                if (err != CURLM_OK) throw httpclient_exception(TRACEMSG(
                        "cURL multi_perform error: [" + curl_multi_strerror(err) + "], url: [" + url + "]"));
                open = (1 == active);
            }

            check_state_after_perform(start);
        }
    }

    void check_state_after_perform(std::chrono::time_point<std::chrono::system_clock> start) {
        // check whether error happened
        if(!error.empty()) {
            throw httpclient_exception(TRACEMSG(error + "\n" +
                    "Processing error for url: [" + url + "]"));
        }
        
        // check whether connection was successful
        if (options.abort_on_connect_error && !open && resource_state::created == state) {
            throw httpclient_exception(TRACEMSG(
                    "Connection error for url: [" + url + "]"));
        }

        // check for response code
        if (options.abort_on_response_error && status_code >= 400) {
            throw httpclient_exception(TRACEMSG(
                    "HTTP error returned from server, url: [" + url + "]," +
                    " status_code: [" + sc::to_string(status_code) + "]"));
        }

        // check for timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
        if (elapsed.count() > options.timeout_millis) {
            throw httpclient_exception(TRACEMSG(
                    "Request timeout for url: [" + url + "]," +
                    " elapsed time (millis): [" + sc::to_string(elapsed.count()) + "]," +
                    " limit: [" + sc::to_string(options.timeout_millis) + "]"));
        }
    }
};

PIMPL_FORWARD_CONSTRUCTOR(single_threaded_http_resource, (const http_session_options&)(const std::string&)(std::unique_ptr<std::istream>)(http_request_options), (), httpclient_exception)
PIMPL_FORWARD_METHOD(single_threaded_http_resource, std::streamsize, read, (sc::span<char>), (), httpclient_exception)
PIMPL_FORWARD_METHOD(single_threaded_http_resource, const std::string&, get_url, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(single_threaded_http_resource, uint16_t, get_status_code, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(single_threaded_http_resource, http_resource_info, get_info, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(single_threaded_http_resource, headers_type, get_headers, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(single_threaded_http_resource, const std::string&, get_header, (const std::string&), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(single_threaded_http_resource, bool, connection_successful, (), (const), httpclient_exception)

} // namespace
}
