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
 * File:   multi_threaded_http_resource.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:43 AM
 */

#include "multi_threaded_http_resource.hpp"

#include <cstring>
#include <chrono>
#include <ios>
#include <limits>
#include <memory>
#include <thread>

#include "staticlib/config.hpp"
#include "staticlib/concurrent.hpp"
#include "staticlib/pimpl/pimpl_forward_macros.hpp"

#include "staticlib/httpclient/http_resource_info.hpp"
#include "staticlib/httpclient/httpclient_exception.hpp"

#include "http_resource_impl.hpp"
#include "http_resource_params.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace sc = staticlib::config;
namespace cr = staticlib::concurrent;

using headers_type = const std::vector<std::pair<std::string, std::string>>&;

} // namespace


class multi_threaded_http_resource::impl : public http_resource::impl {
    std::string url;

    mutable std::shared_ptr<running_request_pipe> pipe;
    mutable std::vector<std::pair<std::string, std::string>> headers;
    
    cr::growing_buffer current_buf;
    size_t start_idx = 0;
    
public:
    impl(http_resource_params&& params):
    http_resource::impl(),
    url(params.url.data(), params.url.length()),
    pipe(std::move(params.pipe)) { }
    
    virtual std::streamsize read(http_resource&, sc::span<char> span) override {
        size_t avail = current_buf.size() - start_idx;
        if (avail > 0) {
            return read_from_current(span, avail);
        }
        start_idx = 0;
        bool success = pipe->receive_some_data(current_buf);
        if (pipe->has_errors()) {
            throw httpclient_exception(TRACEMSG(pipe->get_error_message()));
        }
        if (success) {
            return read_from_current(span, current_buf.size());
        } else {
            return std::char_traits<char>::eof();
        }
    }
    
    virtual const std::string& get_url(const http_resource&) const override {
        return url;
    }
    
    virtual uint16_t get_status_code(const http_resource&) const override {
        return pipe->get_response_code();
    }
    
    virtual http_resource_info get_info(const http_resource&) const override {
        return pipe->get_resource_info();
    }

    virtual const std::vector<std::pair<std::string, std::string>>& get_headers(const http_resource&) const override {
        load_more_headers();
        return headers;
    }

    virtual const std::string& get_header(const http_resource&, const std::string& name) const override {
        // try cached first
        for (auto& en : headers) {
            if (name == en.first) {
                return en.second;
            }
        }
        // load more if available
        load_more_headers();
        for (auto& en : headers) {
            if (name == en.first) {
                return en.second;
            }
        }
        static std::string empty;
        return empty;
    }

    virtual bool connection_successful(const http_resource& frontend) const override {
        return get_status_code(frontend) > 0;
    }
    
private:
    std::streamsize read_from_current(sc::span<char>& span, size_t avail) {
        size_t len = avail <= span.size() ? avail : span.size();
        std::memcpy(span.data(), current_buf.data() + start_idx, len);
        start_idx += len;
        return static_cast<std::streamsize> (len);
    }
    
    void load_more_headers() const {
        auto received = pipe->consume_received_headers();
        for (auto&& he : received) {
            headers.emplace_back(std::move(he));
        }
    }
};
PIMPL_FORWARD_CONSTRUCTOR(multi_threaded_http_resource, (http_resource_params&&), (), httpclient_exception)
PIMPL_FORWARD_METHOD(multi_threaded_http_resource, std::streamsize, read, (sc::span<char>), (), httpclient_exception)
PIMPL_FORWARD_METHOD(multi_threaded_http_resource, const std::string&, get_url, (), (const), httpclient_exception)        
PIMPL_FORWARD_METHOD(multi_threaded_http_resource, uint16_t, get_status_code, (), (const), httpclient_exception)        
PIMPL_FORWARD_METHOD(multi_threaded_http_resource, http_resource_info, get_info, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(multi_threaded_http_resource, headers_type, get_headers, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(multi_threaded_http_resource, const std::string&, get_header, (const std::string&), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(multi_threaded_http_resource, bool, connection_successful, (), (const), httpclient_exception)

} // namespace
}
