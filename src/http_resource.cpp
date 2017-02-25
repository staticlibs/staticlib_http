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
 * File:   http_resource.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:43 AM
 */

#include "staticlib/httpclient/http_resource.hpp"

#include <cstring>
#include <chrono>
#include <ios>
#include <limits>
#include <memory>
#include <thread>

#include "curl/curl.h"

#include "staticlib/config.hpp"
#include "staticlib/concurrent.hpp"
#include "staticlib/pimpl/pimpl_forward_macros.hpp"

#include "staticlib/httpclient/http_resource_info.hpp"
#include "staticlib/httpclient/httpclient_exception.hpp"

#include "http_resource_params.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace sc = staticlib::config;
namespace cr = staticlib::concurrent;

using headers_type = const std::vector<std::pair<std::string, std::string>>&;

} // namespace


class http_resource::impl : public staticlib::pimpl::pimpl_object::impl {
    std::string url;

    mutable std::shared_ptr<running_request_pipe> pipe;
    mutable std::vector<std::pair<std::string, std::string>> headers;
    
    cr::growing_buffer current_buf;
    size_t start_idx = 0;
    
public:
    impl(http_resource_params&& params):
    url(params.url.data(), params.url.length()),
    pipe(std::move(params.pipe)) { }
    
    std::streamsize read(http_resource&, sc::span<char> span) {
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
    
    const std::string& get_url(const http_resource&) const {
        return url;
    }
    
    uint16_t get_status_code(const http_resource&) const {
        return pipe->get_response_code();
    }
    
    http_resource_info get_info(const http_resource&) const {
        return pipe->get_resource_info();
    }

    const std::vector<std::pair<std::string, std::string>>& get_headers(const http_resource&) const {
        load_more_headers();
        return headers;
    }

    const std::string& get_header(const http_resource&, const std::string& name) const {
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

    /**
     * Inspect connection attempt results
     * 
     * @return 'true' if connection has been established successfully and
     *         response code has been received, 'false' otherwise
     */
    bool connection_successful(const http_resource& parent) const {
        return get_status_code(parent) > 0;
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

PIMPL_FORWARD_CONSTRUCTOR(http_resource, (http_resource_params&&), (), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, std::streamsize, read, (sc::span<char>), (), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, const std::string&, get_url, (), (const), httpclient_exception)        
PIMPL_FORWARD_METHOD(http_resource, uint16_t, get_status_code, (), (const), httpclient_exception)        
PIMPL_FORWARD_METHOD(http_resource, http_resource_info, get_info, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, headers_type, get_headers, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, const std::string&, get_header, (const std::string&), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, bool, connection_successful, (), (const), httpclient_exception)

} // namespace
}
