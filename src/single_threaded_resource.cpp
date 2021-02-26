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
 * File:   single_threaded_resource.cpp
 * Author: alex
 *
 * Created on March 25, 2017, 10:08 PM
 */

#include "single_threaded_resource.hpp"

#include <chrono>
#include <thread>
#include <vector>

#include "curl/curl.h"

#include "staticlib/io.hpp"
#include "staticlib/pimpl/forward_macros.hpp"
#include "staticlib/utils.hpp"

#include "curl_deleters.hpp"
#include "curl_headers.hpp"
#include "curl_info.hpp"
#include "curl_options.hpp"
#include "curl_utils.hpp"
#include "resource_impl.hpp"

namespace staticlib {
namespace http {

namespace { // anonymous

using headers_type = const std::vector<std::pair<std::string, std::string>>&;
using fin_type = std::function<void()>;

} // namespace

class single_threaded_resource::impl : public resource::impl {
    enum class resource_state {
        created,
        writing_headers,
        response_info_filled
    };

    uint64_t id;
    CURLM* multi_handle;
    std::unique_ptr<CURL, curl_easy_deleter> handle;

    // holds data passed to curl
    std::string url;
    session_options session_opts;
    request_options options;
    std::unique_ptr<std::istream> post_data;
    curl_headers request_headers;

    // run details
    resource_info info;
    uint16_t status_code = 0;
    std::vector<std::pair<std::string, std::string>> response_headers;
    std::vector<char> buf;
    size_t buf_idx = 0;
    bool open = false;
    resource_state state = resource_state::created;
    std::string error;

public:
    impl(uint64_t resource_id, CURLM* multi_handle, const session_options& session_opts,
            const std::string& url, std::unique_ptr<std::istream> post_data,
            request_options options, std::function<void()> finalizer) :
    id(resource_id),
    multi_handle(multi_handle),
    handle(curl_easy_init(), curl_easy_deleter(this->multi_handle, finalizer)),
    url(url.data(), url.length()),
    session_opts(session_opts),
    options(std::move(options)),
    post_data(std::move(post_data)) {
        CURLMcode errm = curl_multi_add_handle(multi_handle, handle.get());
        if (errm != CURLM_OK) throw http_exception(TRACEMSG(
                "cURL multi_add error: [" + curl_multi_strerror(errm) + "], url: [" + this->url + "]"));
        apply_curl_options(this, this->url, this->options, this->post_data, this->request_headers, this->handle);
        this->open = true;
        fill_buffer();
    }

    virtual std::streamsize read(resource&, sl::io::span<char> span) override {
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

    virtual const std::string& get_url(const resource&) const override {
        return url;
    }

    virtual uint16_t get_status_code(const resource&) const override {
        return status_code;
    }

    virtual resource_info get_info(const resource&) const override {
        return info;
    }

    virtual const std::vector<std::pair<std::string, std::string>>& get_headers(const resource&) const override {
        return response_headers;
    }

    virtual const std::string& get_header(const resource&, const std::string& name) const override {
        for (auto& en : response_headers) {
            if (name == en.first) {
                return en.second;
            }
        }
        return sl::utils::empty_string();
    }

    virtual bool connection_successful(const resource& frontend) const override {
        return get_status_code(frontend) > 0;
    }

    virtual uint64_t get_id(const resource&) const override {
        return id;
    }

    virtual const std::string& get_response_data_file(const resource&) const override {
        return sl::utils::empty_string();
    }

    size_t write_headers(char* buffer, size_t size, size_t nitems) {
        if (resource_state::created == state) {
            curl_info ci(handle.get());
            this->status_code = static_cast<uint16_t>(ci.getinfo_long(CURLINFO_RESPONSE_CODE));
            this->state = resource_state::writing_headers;
        }
        size_t len = size*nitems;
        auto opt = curl_parse_header(buffer, len);
        if (opt) {
            response_headers.emplace_back(std::move(opt.value()));
        }
        return len;
    }

    // todo: verify me, currently this impl is not used
    size_t write_data(char *buffer, size_t size, size_t nitems) {
        size_t len = size*nitems;
        buf.resize(len);
        std::memcpy(buf.data(), buffer, len);
        return len;
    }

    size_t read_data(char* buffer, size_t size, size_t nitems) {
        std::streamsize len = static_cast<std::streamsize> (size * nitems);
        sl::io::streambuf_source src{post_data->rdbuf()};
        std::streamsize read = sl::io::read_all(src,{buffer, len});
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
            CURLMcode err_timeout = curl_multi_timeout(multi_handle, std::addressof(timeo));
            if (err_timeout != CURLM_OK) throw http_exception(TRACEMSG(
                    "cURL timeout error: [" + curl_multi_strerror(err_timeout) + "], url: [" + url + "]"));
            struct timeval timeout = create_timeout_struct(timeo, session_opts.st_socket_select_max_timeout_millis);

            // get file descriptors from the transfers
            fd_set fdread = create_fd();
            fd_set fdwrite = create_fd();
            fd_set fdexcep = create_fd();
            int maxfd = -1;
            CURLMcode err_fdset = curl_multi_fdset(multi_handle, std::addressof(fdread),
                    std::addressof(fdwrite), std::addressof(fdexcep), std::addressof(maxfd));
            if (err_fdset != CURLM_OK) throw http_exception(TRACEMSG(
                    "cURL fdset error: [" + curl_multi_strerror(err_fdset) + "], url: [" + url + "]"));

            // wait or select
            int err_select = 0;
            if (maxfd == -1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(session_opts.fdset_timeout_millis));
            } else {
                err_select = select(maxfd + 1, std::addressof(fdread), std::addressof(fdwrite),
                        std::addressof(fdexcep), std::addressof(timeout));
            }

            // do perform if no select error
            if (-1 != err_select) {
                int active = -1;
                CURLMcode err = curl_multi_perform(multi_handle, std::addressof(active));
                if (err != CURLM_OK) throw http_exception(TRACEMSG(
                        "cURL multi_perform error: [" + curl_multi_strerror(err) + "], url: [" + url + "]"));
                open = (1 == active);
            }

            check_state_after_perform(start);
        }
    }

    void check_state_after_perform(std::chrono::time_point<std::chrono::system_clock> start) {
        // check whether error happened
        if(!error.empty()) {
            throw http_exception(TRACEMSG(error + "\n" +
                    "Processing error for url: [" + url + "]"));
        }
        
        // check whether connection was successful
        if (options.abort_on_connect_error && !open && resource_state::created == state) {
            throw http_exception(TRACEMSG(
                    "Connection error for url: [" + url + "]"));
        }

        // check for response code
        if (options.abort_on_response_error && status_code >= 400) {
            throw http_exception(TRACEMSG(
                    "HTTP error returned from server, url: [" + url + "]," +
                    " status_code: [" + sl::support::to_string(status_code) + "]"));
        }

        // check for timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
        if (elapsed.count() > options.timeout_millis) {
            throw http_exception(TRACEMSG(
                    "Request timeout for url: [" + url + "]," +
                    " elapsed time (millis): [" + sl::support::to_string(elapsed.count()) + "]," +
                    " limit: [" + sl::support::to_string(options.timeout_millis) + "]"));
        }
    }
};

PIMPL_FORWARD_CONSTRUCTOR(single_threaded_resource, (uint64_t)(CURLM*)(const session_options&)(const std::string&)(std::unique_ptr<std::istream>)(request_options)(fin_type), (), http_exception)
PIMPL_FORWARD_METHOD(single_threaded_resource, std::streamsize, read, (sl::io::span<char>), (), http_exception)
PIMPL_FORWARD_METHOD(single_threaded_resource, const std::string&, get_url, (), (const), http_exception)
PIMPL_FORWARD_METHOD(single_threaded_resource, uint16_t, get_status_code, (), (const), http_exception)
PIMPL_FORWARD_METHOD(single_threaded_resource, resource_info, get_info, (), (const), http_exception)
PIMPL_FORWARD_METHOD(single_threaded_resource, headers_type, get_headers, (), (const), http_exception)
PIMPL_FORWARD_METHOD(single_threaded_resource, const std::string&, get_header, (const std::string&), (const), http_exception)
PIMPL_FORWARD_METHOD(single_threaded_resource, bool, connection_successful, (), (const), http_exception)
PIMPL_FORWARD_METHOD(single_threaded_resource, uint64_t, get_id, (), (const), http_exception)
PIMPL_FORWARD_METHOD(single_threaded_resource, const std::string&, get_response_data_file, (), (const), http_exception)

} // namespace
}
