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
 * File:   session_impl.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 5:45 PM
 */

#ifndef STATICLIB_HTTP_SESSION_IMPL_HPP
#define STATICLIB_HTTP_SESSION_IMPL_HPP

#include "staticlib/http/session.hpp"

#include "curl/curl.h"

#include "curl_deleters.hpp"

namespace staticlib {
namespace http {

class session::impl : public sl::pimpl::object::impl {
protected:
    session_options options;
    std::unique_ptr<CURLM, curl_multi_deleter> handle;

public:
    impl(session_options opts = session_options{});

    resource open_url(
            session&,
            const std::string& url,
            request_options opts);

    resource open_url(
            session&,
            const std::string& url,
            std::streambuf* post_data,
            request_options opts);

};

} // namespace
}

#endif /* STATICLIB_HTTP_SESSION_IMPL_HPP */

