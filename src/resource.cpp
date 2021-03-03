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
 * File:   resource.cpp
 * Author: alex
 *
 * Created on March 25, 2017, 9:32 PM
 */

#include "resource_impl.hpp"

#include "staticlib/pimpl/forward_macros.hpp"

namespace staticlib {
namespace http {

namespace { // anonymous

using headers_type = const std::vector<std::pair<std::string, std::string>>&;

} // namespace

PIMPL_FORWARD_METHOD(resource, std::streamsize, read, (sl::io::span<char>), (), http_exception)
PIMPL_FORWARD_METHOD(resource, const std::string&, get_url, (), (const), http_exception)
PIMPL_FORWARD_METHOD(resource, uint16_t, get_status_code, (), (const), http_exception)
PIMPL_FORWARD_METHOD(resource, resource_info, get_info, (), (const), http_exception)
PIMPL_FORWARD_METHOD(resource, headers_type, get_headers, (), (const), http_exception)
PIMPL_FORWARD_METHOD(resource, const std::string&, get_header, (const std::string&), (const), http_exception)
PIMPL_FORWARD_METHOD(resource, bool, connection_successful, (), (const), http_exception)
PIMPL_FORWARD_METHOD(resource, uint64_t, get_id, (), (const), http_exception)
PIMPL_FORWARD_METHOD(resource, const request_options&, get_request_options, (), (const), http_exception)
PIMPL_FORWARD_METHOD(resource, const std::string&, get_error, (), (const), http_exception)

} // namespace
}
