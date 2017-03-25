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
 * File:   http_resource_impl.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 9:17 PM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTP_RESOURCE_IMPL_HPP
#define	STATICLIB_HTTPCLIENT_HTTP_RESOURCE_IMPL_HPP

#include "staticlib/httpclient/http_resource.hpp"

namespace staticlib {
namespace httpclient {

class http_resource::impl : public staticlib::pimpl::pimpl_object::impl {
    
public:
    virtual std::streamsize read(http_resource&, staticlib::config::span<char> span) = 0;

    virtual const std::string& get_url(const http_resource&) const = 0;

    virtual uint16_t get_status_code(const http_resource&) const = 0;

    virtual http_resource_info get_info(const http_resource&) const = 0;

    virtual const std::vector<std::pair<std::string, std::string>>& get_headers(const http_resource&) const = 0;

    virtual const std::string& get_header(const http_resource&,const std::string& name) const = 0;

    virtual bool connection_successful(const http_resource&) const = 0;
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTP_RESOURCE_IMPL_HPP */

