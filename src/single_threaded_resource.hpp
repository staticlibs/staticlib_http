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
 * File:   single_threaded_resource.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 9:43 PM
 */

#ifndef STATICLIB_HTTP_SINGLE_THREADED_RESOURCE_HPP
#define STATICLIB_HTTP_SINGLE_THREADED_RESOURCE_HPP

#include "staticlib/http/resource.hpp"

#include <functional>

#include "curl/curl.h"

#include "staticlib/http/session_options.hpp"

namespace staticlib {
namespace http {

class single_threaded_resource : public resource {
protected:
    class impl;

public:
    PIMPL_INHERIT_CONSTRUCTOR(single_threaded_resource, resource)

    single_threaded_resource(CURLM* multi_handle, const session_options& session_options, 
            const std::string& url, std::unique_ptr<std::istream> post_data,
            request_options options, std::function<void()> finalizer);

    virtual std::streamsize read(sl::io::span<char> span) override;

    virtual const std::string& get_url() const override;

    virtual uint16_t get_status_code() const override;

    virtual resource_info get_info() const override;

    virtual const std::vector<std::pair<std::string, std::string>>& get_headers() const override;

    virtual const std::string& get_header(const std::string& name) const override;

    virtual bool connection_successful() const override;    

};

} // namespace
}

#endif /* STATICLIB_HTTP_SINGLE_THREADED_RESOURCE_HPP */

