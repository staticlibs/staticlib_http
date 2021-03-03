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
 * File:   polling_resource.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:43 AM
 */

#include "polling_resource.hpp"

#include <cstring>
#include <chrono>
#include <ios>
#include <limits>

#include "staticlib/config.hpp"
#include "staticlib/concurrent.hpp"
#include "staticlib/pimpl/forward_macros.hpp"
#include "staticlib/utils.hpp"

#include "staticlib/http/request_options.hpp"
#include "staticlib/http/resource_info.hpp"
#include "staticlib/http/http_exception.hpp"

#include "resource_impl.hpp"

namespace staticlib {
namespace http {

namespace { // anonymous

using headers_type = std::vector<std::pair<std::string, std::string>>;

} // namespace

class polling_resource::impl : public resource::impl {
    uint64_t id;
    request_options request_opts;
    std::string url;
    bool empty;

    resource_info info;
    uint16_t status_code = 0;
    std::vector<std::pair<std::string, std::string>> response_headers;
    std::vector<char> buf;
    size_t buf_idx = 0;
    std::string error;

public:
    impl(uint64_t resource_id, const request_options& req_options, const std::string& url):
    resource::impl(),
    id(resource_id),
    request_opts(req_options),
    url(url.data(), url.length()),
    empty(true) { }

    impl(uint64_t resource_id, const request_options& req_options, const std::string& url,
            resource_info&& info, uint16_t status_code,
            std::vector<std::pair<std::string, std::string>>&& response_headers,
            std::vector<char>&& data, const std::string& error_message):
    resource::impl(),
    id(resource_id),
    request_opts(req_options),
    url(url.data(), url.length()),
    empty(false),
    info(std::move(info)),
    status_code(status_code),
    response_headers(std::move(response_headers)),
    buf(std::move(data)),
    error(error_message.data(), error_message.length()){
        if (0 == status_code && error.empty()) {
            error.append("Connection error");
        }
    }

    virtual std::streamsize read(resource&, sl::io::span<char> span) override {
        if (!empty) {
            // return from buffer
            if (buf_idx < buf.size()) {
                size_t ulen = span.size();
                size_t avail = buf.size() - buf_idx;
                size_t reslen = avail <= ulen ? avail : ulen;
                std::memcpy(span.data(), buf.data() + buf_idx, reslen);
                buf_idx += reslen;
                return static_cast<std::streamsize> (reslen);
            } else {
                return std::char_traits<char>::eof();
            }
        } else {
            return std::char_traits<char>::eof();
        }
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
        return !empty && get_status_code(frontend) > 0;
    }

    virtual uint64_t get_id(const resource&) const override {
        return id;
    }

    virtual const request_options& get_request_options(const resource&) const override {
        return request_opts;
    }

    virtual const std::string& get_error(const resource&) const override {
        return error;
    }
};
PIMPL_FORWARD_CONSTRUCTOR(polling_resource, (uint64_t)(const request_options&)(const std::string&), (), http_exception)
PIMPL_FORWARD_CONSTRUCTOR(polling_resource, (uint64_t)(const request_options&)(const std::string&)(resource_info&&)(uint16_t)(headers_type&&)(std::vector<char>&&)(const std::string&), (), http_exception)
PIMPL_FORWARD_METHOD(polling_resource, std::streamsize, read, (sl::io::span<char>), (), http_exception)
PIMPL_FORWARD_METHOD(polling_resource, const std::string&, get_url, (), (const), http_exception)
PIMPL_FORWARD_METHOD(polling_resource, uint16_t, get_status_code, (), (const), http_exception)
PIMPL_FORWARD_METHOD(polling_resource, resource_info, get_info, (), (const), http_exception)
PIMPL_FORWARD_METHOD(polling_resource, const headers_type&, get_headers, (), (const), http_exception)
PIMPL_FORWARD_METHOD(polling_resource, const std::string&, get_header, (const std::string&), (const), http_exception)
PIMPL_FORWARD_METHOD(polling_resource, bool, connection_successful, (), (const), http_exception)
PIMPL_FORWARD_METHOD(polling_resource, uint64_t, get_id, (), (const), http_exception)
PIMPL_FORWARD_METHOD(polling_resource, const request_options&, get_request_options, (), (const), http_exception)
PIMPL_FORWARD_METHOD(polling_resource, const std::string&, get_error, (), (const), http_exception)

} // namespace
}
