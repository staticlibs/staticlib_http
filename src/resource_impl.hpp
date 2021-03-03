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
 * File:   resource_impl.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 9:17 PM
 */

#ifndef STATICLIB_HTTP_RESOURCE_IMPL_HPP
#define STATICLIB_HTTP_RESOURCE_IMPL_HPP

#include "staticlib/http/resource.hpp"

#include "staticlib/http/request_options.hpp"

namespace staticlib {
namespace http {

class resource::impl : public sl::pimpl::object::impl {

public:
    virtual std::streamsize read(resource&, sl::io::span<char> span) = 0;

    virtual const std::string& get_url(const resource&) const = 0;

    virtual uint16_t get_status_code(const resource&) const = 0;

    virtual resource_info get_info(const resource&) const = 0;

    virtual const std::vector<std::pair<std::string, std::string>>& get_headers(const resource&) const = 0;

    virtual const std::string& get_header(const resource&,const std::string& name) const = 0;

    virtual bool connection_successful(const resource&) const = 0;

    virtual uint64_t get_id(const resource&) const = 0;

    virtual const request_options& get_request_options(const resource&) const = 0;

    virtual const std::string& get_error(const resource&) const = 0;
};

} // namespace
}

#endif /* STATICLIB_HTTP_RESOURCE_IMPL_HPP */

