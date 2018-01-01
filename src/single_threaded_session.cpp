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
 * File:   single_threaded_session.cpp
 * Author: alex
 *
 * Created on March 25, 2017, 9:48 PM
 */

#include "staticlib/http/single_threaded_session.hpp"

#include "curl/curl.h"

#include "staticlib/pimpl/forward_macros.hpp"

#include "session_impl.hpp"
#include "curl_headers.hpp"
#include "curl_utils.hpp"
#include "single_threaded_resource.hpp"

namespace staticlib {
namespace http {

class single_threaded_session::impl : public session::impl {
    bool has_active_request = false;
    
public:
    impl(session_options opts) :
    session::impl(opts) { }
    resource open_url(single_threaded_session&, const std::string& url,
            std::unique_ptr<std::istream> post_data, request_options opts) {
        if (has_active_request) throw http_exception(TRACEMSG(
                "This single-threaded session is already has one HTTP resource open, please dispose it first"));
        if ("" == opts.method) {
            opts.method = "POST";
        }
        this->has_active_request = true;
        return single_threaded_resource(handle.get(), this->options, std::move(url), 
                std::move(post_data), std::move(opts), [this] {this->has_active_request = false; });
    }
    
};

PIMPL_FORWARD_CONSTRUCTOR(single_threaded_session, (session_options), (), http_exception)
PIMPL_FORWARD_METHOD(single_threaded_session, resource, open_url, (const std::string&)(std::unique_ptr<std::istream>)(request_options), (), http_exception)

} // namespace
}
