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
 * File:   basic_http_session_impl.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 5:45 PM
 */

#ifndef STATICLIB_HTTPCLIENT_BASIC_HTTP_SESSION_IMPL_HPP
#define	STATICLIB_HTTPCLIENT_BASIC_HTTP_SESSION_IMPL_HPP

#include "staticlib/httpclient/basic_http_session.hpp"

#include "curl/curl.h"

#include "curl_deleters.hpp"

namespace staticlib {
namespace httpclient {

class basic_http_session::impl : public staticlib::pimpl::pimpl_object::impl {
protected:
    http_session_options options;

public:
    impl(http_session_options options = http_session_options{});
    
    http_resource open_url(
            basic_http_session&,
            const std::string& url,
            http_request_options options);

    http_resource open_url(
            basic_http_session&,
            const std::string& url,
            std::streambuf* post_data,
            http_request_options options);
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_BASIC_HTTP_SESSION_IMPL_HPP */

