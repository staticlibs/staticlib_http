/*
 * Copyright 2021, alex at staticlibs.net
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
 * File:   polling_session.cpp
 * Author: alex
 *
 * Created on January 17, 2021, 1:23 PM
 */
#include "staticlib/http/polling_session.hpp"

#include <cstdint>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "curl/curl.h"

#include "staticlib/support.hpp"
#include "staticlib/io.hpp"
#include "staticlib/pimpl/forward_macros.hpp"
#include "staticlib/tinydir.hpp"

#include "session_impl.hpp"
#include "curl_headers.hpp"
#include "curl_utils.hpp"
#include "resource_params.hpp"
#include "polling_resource.hpp"
#include "running_request_pipe.hpp"
#include "running_request.hpp"

namespace staticlib {
namespace http {

namespace { // anonymous

class request {
    uint64_t id;

    std::unique_ptr<CURL, curl_easy_deleter> handle;

    // holds data passed to curl
    std::string url;
    request_options options;
    std::unique_ptr<std::istream> post_data;
    curl_headers request_headers;

    // run details
    resource_info info;
    uint16_t status_code = 0;
    std::vector<std::pair<std::string, std::string>> response_headers;
    std::vector<char> buf;
    std::unique_ptr<sl::tinydir::file_sink> response_body_file_sink;
    std::string error;

    // no-copy
    request(const request&) = delete;
    void operator=(const request&) = delete;

public:
    request(uint64_t request_id, std::unique_ptr<CURL, curl_easy_deleter> handle, const std::string& url,
            std::unique_ptr<std::istream> post_data, request_options opts):
    id(request_id),
    handle(std::move(handle)),
    url(url.data(), url.length()),
    options(std::move(opts)),
    post_data(std::move(post_data)) {
        if (!this->options.polling_response_body_file_path.empty()) {
            this->response_body_file_sink = sl::support::make_unique<sl::tinydir::file_sink>(
                    this->options.polling_response_body_file_path);
        }
        apply_curl_options(this, this->url, this->options, this->post_data, this->request_headers, this->handle);
    }

    size_t write_headers(char* buffer, size_t size, size_t nitems) {
        if (status_code < 200) {
            curl_info ci(handle.get());
            this->status_code = static_cast<uint16_t>(ci.getinfo_long(CURLINFO_RESPONSE_CODE));
        }
        size_t len = size*nitems;
        auto opt = curl_parse_header(buffer, len);
        if (opt) {
            response_headers.emplace_back(std::move(opt.value()));
        }
        return len;
    }

    size_t write_data(char* buffer, size_t size, size_t nitems) {
        size_t len = size*nitems;
        if (nullptr == response_body_file_sink.get()) {
            size_t buf_size = buf.size();
            size_t max_size = options.polling_response_body_max_size_bytes;
            if (max_size > 0 && buf_size + len > max_size) {
                this->status_code = 0;
                this->append_error(std::string() + "response body size exceeded, " +
                        "limit: [" + sl::support::to_string(max_size) + "]");
                return 0;
            }
            buf.resize(buf_size + len);
            std::memcpy(buf.data() + buf_size, buffer, len);
        } else {
            sl::io::write_all(*response_body_file_sink, {buffer, len});
        }
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

    polling_resource to_resource() {
        auto info = curl_collect_info(handle.get());
        if (nullptr != response_body_file_sink.get()) {
            response_body_file_sink.reset();
        }
        return polling_resource(id, url, std::move(info), status_code,
                std::move(response_headers), std::move(buf), options.polling_response_body_file_path,
                error);
    }
};

} // namespace

class polling_session::impl : public session::impl {
    std::map<int64_t, std::unique_ptr<request>> queue;

public:
    impl(session_options opts) :
    session::impl(opts) { }

    ~impl() STATICLIB_NOEXCEPT {
    }

    resource open_url(polling_session&, const std::string& url,
            std::unique_ptr<std::istream> post_data, request_options opts) {
        if ("" == opts.method) {
            opts.method = "POST";
        }

        // create easy handle
        auto easy_handle = std::unique_ptr<CURL, curl_easy_deleter>(
                curl_easy_init(), curl_easy_deleter(this->handle.get()));
        if (nullptr == easy_handle.get()) throw http_exception(TRACEMSG(
                "Error creating cURL handle, url: [" + url + "]," +
                " queue size: [" + sl::support::to_string(queue.size()) + "]"));

        // enqueue request
        CURLMcode errm = curl_multi_add_handle(handle.get(), easy_handle.get());
        if (errm != CURLM_OK) throw http_exception(TRACEMSG(
                "cURL multi_add error: [" + curl_multi_strerror(errm) + "], url: [" + url + "]"));

        auto id = increment_resource_id();
        auto key = reinterpret_cast<int64_t>(easy_handle.get());
        auto req = sl::support::make_unique<request>(id, std::move(easy_handle), url, std::move(post_data), opts);
        auto inserted = queue.insert(std::make_pair(key, std::move(req)));
        if (!inserted.second) throw http_exception(TRACEMSG(
                "Error enqueuing cURL handle, url: [" + url + "]," +
                " queue size: [" + sl::support::to_string(queue.size()) + "]"));

        // return empty resource
        return polling_resource(id, url);
    }

    std::vector<resource> poll(polling_session&) {
        auto results = std::vector<resource>();
        
        if (0 == queue.size()) {
            return results;
        }
 
        // timeout
        auto timeout = call_timeout();

        // select
        auto can_perform = call_select(timeout);
        if (!can_perform) {
            return results;
        }

        // perform
        auto active = call_perform();

        // collect finished
        if (active < queue.size()) {
            CURL* easy_handle = nullptr;
            while(nullptr != (easy_handle = call_info())) {
                auto key = reinterpret_cast<int64_t>(easy_handle);
                auto req = dequeue_request(key);
                auto res = req->to_resource();
                results.emplace_back(std::move(res));
            }
        }

        // sanity check
        if (queue.size() != active) throw http_exception(TRACEMSG(
                "Polling session system error: inconsistent queue state," +
                " active transfers count: [" + sl::support::to_string(active) + "],"
                " queue size: [" + sl::support::to_string(queue.size()) + "]," +
                " results count: [" + sl::support::to_string(results.size()) + "]"));

        return results;
    }

    struct timeval call_timeout() {
        long timeo = -1;
        CURLMcode err = curl_multi_timeout(this->handle.get(), std::addressof(timeo));
        if (CURLM_OK != err) throw http_exception(TRACEMSG(
                "cURL multi_timeout error: [" + curl_multi_strerror(err) + "]"));
        return create_timeout_struct(timeo, this->options.mt_socket_select_max_timeout_millis);
    }

    bool call_select(struct timeval& timeout) {
        // prepare descriptors
        fd_set fdread = create_fd();
        fd_set fdwrite = create_fd();
        fd_set fdexcep = create_fd();
        int maxfd = -1;
        CURLMcode err_fdset = curl_multi_fdset(this->handle.get(), std::addressof(fdread),
                std::addressof(fdwrite), std::addressof(fdexcep), std::addressof(maxfd));
        if (CURLM_OK != err_fdset) throw http_exception(TRACEMSG(
                "cURL multi_fdset error: [" + curl_multi_strerror(err_fdset) + "]"));

        // call select
        int err_select = 0;
        if (maxfd == -1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(this->options.fdset_timeout_millis));
        } else {
            err_select = select(maxfd + 1, std::addressof(fdread), std::addressof(fdwrite),
                    std::addressof(fdexcep), std::addressof(timeout));
        }
        return -1 != err_select;
    }

    size_t call_perform() {
        int active = 0;
        CURLMcode err = curl_multi_perform(this->handle.get(), std::addressof(active));
        if (CURLM_OK != err) throw http_exception(TRACEMSG(
                "cURL multi_perform error: [" + curl_multi_strerror(err) + "]"));
        return static_cast<size_t>(active);
    }

    CURL* call_info() {
        int msg_count = -1;
        auto msg = curl_multi_info_read(this->handle.get(), std::addressof(msg_count));
        if (nullptr == msg) {
            return nullptr;
        }
        if (CURLMSG_DONE != msg->msg) throw http_exception(TRACEMSG(
                "cURL multi_info_read error, msg_count: [" + sl::support::to_string(msg_count) + "]"));
        // todo: collect and append error
        return msg->easy_handle;
    }

    std::unique_ptr<request> dequeue_request(int64_t key) {
        auto it = queue.find(key);
        if (queue.end() == it) throw http_exception(TRACEMSG(
                "Dequeue find error, key: [" + sl::support::to_string(key) + "]"));
        auto res = std::move(it->second);
        auto count = queue.erase(key);
        if (1 != count) throw http_exception(TRACEMSG(
                "Dequeue erase error, key: [" + sl::support::to_string(key) + "]"));
        return res;
    }

};
PIMPL_FORWARD_CONSTRUCTOR(polling_session, (session_options), (), http_exception)
PIMPL_FORWARD_METHOD(polling_session, resource, open_url, (const std::string&)(std::unique_ptr<std::istream>)(request_options), (), http_exception)
PIMPL_FORWARD_METHOD(polling_session, std::vector<resource>, poll, (), (), http_exception)

} // namespace
}