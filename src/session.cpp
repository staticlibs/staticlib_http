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
 * File:   session.cpp
 * Author: alex
 *
 * Created on March 25, 2017, 5:45 PM
 */

#include "session_impl.hpp"

#include "staticlib/pimpl/forward_macros.hpp"

#include "curl_options.hpp"

namespace staticlib {
namespace http {

session::impl::impl(session_options opts) :
// todo: use move
options(opts),
handle(curl_multi_init(), curl_multi_deleter()) {
    if (nullptr == handle.get()) throw http_exception(TRACEMSG("Error initializing cURL multi handle"));
    apply_curl_multi_options(this->handle.get(), this->options);
}
PIMPL_FORWARD_CONSTRUCTOR(session, (session_options), (), http_exception)

resource session::impl::open_url(session& frontend, 
        const std::string& url, request_options opts) {
    if ("" == opts.method) {
        opts.method = "GET";
    }
    return frontend.open_url(url, static_cast<std::streambuf*>(nullptr), std::move(opts));
}
PIMPL_FORWARD_METHOD(session, resource, open_url, (const std::string&)(request_options), (), http_exception)


resource session::impl::open_url(session& frontend,
        const std::string& url, std::streambuf* post_data, request_options opts) {
    auto sbuf_ptr = [post_data] () -> std::streambuf* {
        if (nullptr != post_data) {
            return post_data;
        }
        static std::istringstream empty_stream{""};
        return empty_stream.rdbuf();
    } ();
    if ("" == opts.method) {
        opts.method = "POST";
    }
    auto sbuf = sl::io::make_source_istream_ptr(sl::io::streambuf_source(sbuf_ptr));
    return frontend.open_url(url, std::move(sbuf), std::move(opts));
}
PIMPL_FORWARD_METHOD(session, resource, open_url, (const std::string&)(std::streambuf*)(request_options), (), http_exception)
        
} // namespace
}
