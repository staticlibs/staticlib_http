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
 * File:   http_resource.cpp
 * Author: alex
 *
 * Created on March 25, 2017, 9:32 PM
 */

#include "http_resource_impl.hpp"

#include "staticlib/pimpl/pimpl_forward_macros.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace sc = staticlib::config;

using headers_type = const std::vector<std::pair<std::string, std::string>>&;

} // namespace

PIMPL_FORWARD_METHOD(http_resource, std::streamsize, read, (sc::span<char>), (), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, const std::string&, get_url, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, uint16_t, get_status_code, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, http_resource_info, get_info, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, headers_type, get_headers, (), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, const std::string&, get_header, (const std::string&), (const), httpclient_exception)
PIMPL_FORWARD_METHOD(http_resource, bool, connection_successful, (), (const), httpclient_exception)

} // namespace
}
