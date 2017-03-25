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
 * File:   basic_http_session.cpp
 * Author: alex
 *
 * Created on March 25, 2017, 5:45 PM
 */

#include "basic_http_session_impl.hpp"

#include "staticlib/pimpl/pimpl_forward_macros.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace sc = staticlib::config;
namespace si = staticlib::io;

} // namespace

basic_http_session::impl::impl(http_session_options options) :
// todo: use move
options(options) { }
PIMPL_FORWARD_CONSTRUCTOR(basic_http_session, (http_session_options), (), httpclient_exception)

http_resource basic_http_session::impl::open_url(basic_http_session& frontend, 
        const std::string& url, http_request_options options) {
    if ("" == options.method) {
        options.method = "GET";
    }
    return frontend.open_url(url, static_cast<std::streambuf*>(nullptr), std::move(options));
}
PIMPL_FORWARD_METHOD(basic_http_session, http_resource, open_url, (const std::string&)(http_request_options), (), httpclient_exception)


http_resource basic_http_session::impl::open_url(basic_http_session& frontend,
        const std::string& url, std::streambuf* post_data, http_request_options options) {
    auto sbuf_ptr = [post_data] () -> std::streambuf* {
        if (nullptr != post_data) {
            return post_data;
        }
        static std::istringstream empty_stream{""};
        return empty_stream.rdbuf();
    } ();
    if ("" == options.method) {
        options.method = "POST";
    }
    auto sbuf = si::make_source_istream_ptr(si::streambuf_source(sbuf_ptr));
    return frontend.open_url(url, std::move(sbuf), std::move(options));
}
PIMPL_FORWARD_METHOD(basic_http_session, http_resource, open_url, (const std::string&)(std::streambuf*)(http_request_options), (), httpclient_exception)
        
} // namespace
}
