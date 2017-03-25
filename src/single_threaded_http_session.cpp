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
 * File:   single_threaded_http_session.cpp
 * Author: alex
 *
 * Created on March 25, 2017, 9:48 PM
 */

#include "staticlib/httpclient/single_threaded_http_session.hpp"

#include "curl/curl.h"

#include "staticlib/pimpl/pimpl_forward_macros.hpp"

#include "basic_http_session_impl.hpp"
#include "curl_headers.hpp"
#include "curl_utils.hpp"
#include "single_threaded_http_resource.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace sc = staticlib::config;
namespace io = staticlib::io;

} // namespace

class single_threaded_http_session::impl : public basic_http_session::impl {    
public:
    impl(http_session_options options) :
    basic_http_session::impl(options) { }
    http_resource open_url(single_threaded_http_session&, const std::string& url,
            std::unique_ptr<std::istream> post_data, http_request_options options) {
        if ("" == options.method) {
            options.method = "POST";
        }
        return single_threaded_http_resource(this->options, std::move(url), 
                std::move(post_data), std::move(options));
    }
    
};

PIMPL_FORWARD_CONSTRUCTOR(single_threaded_http_session, (http_session_options), (), httpclient_exception)
PIMPL_FORWARD_METHOD(single_threaded_http_session, http_resource, open_url, (const std::string&)(std::unique_ptr<std::istream>)(http_request_options), (), httpclient_exception)

} // namespace
}
