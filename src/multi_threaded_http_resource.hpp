/*
 * Copyright 2016, alex at staticlibs.net
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
 * File:   multi_threaded_http_resource.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 9:10 PM
 */

#ifndef STATICLIB_HTTPCLIENT_MULTI_THREADED_HTTP_RESOURCE_HPP
#define	STATICLIB_HTTPCLIENT_MULTI_THREADED_HTTP_RESOURCE_HPP

#include "staticlib/httpclient/http_resource.hpp"

namespace staticlib {
namespace httpclient {

// forward decl
class http_resource_params;

class multi_threaded_http_resource : public http_resource {
protected:
    class impl;
    
public:    
    PIMPL_INHERIT_CONSTRUCTOR(multi_threaded_http_resource, http_resource)

    multi_threaded_http_resource(http_resource_params&& params);
    
    virtual std::streamsize read(staticlib::config::span<char> span) override;

    virtual const std::string& get_url() const override;

    virtual uint16_t get_status_code() const override;

    virtual http_resource_info get_info() const override;

    virtual const std::vector<std::pair<std::string, std::string>>& get_headers() const override;

    virtual const std::string& get_header(const std::string& name) const override;

    virtual bool connection_successful() const override;
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_MULTI_THREADED_HTTP_RESOURCE_HPP */

