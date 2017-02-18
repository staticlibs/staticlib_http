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
 * File:   http_resource_params.hpp
 * Author: alex
 *
 * Created on February 16, 2017, 12:42 PM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTP_RESOURCE_PARAMS_HPP
#define	STATICLIB_HTTPCLIENT_HTTP_RESOURCE_PARAMS_HPP

#include <string>
#include <memory>

#include "running_request_pipe.hpp"

namespace staticlib {
namespace httpclient {

struct http_resource_params {
    const std::string& url;
    std::shared_ptr<running_request_pipe> pipe;
    
    http_resource_params(const std::string& url, std::shared_ptr<running_request_pipe>&& pipe) :
    url(url),
    pipe(std::move(pipe)) { }
    
    http_resource_params(const http_resource_params&) = delete;
    
    http_resource_params& operator=(const http_resource_params&) = delete;
    
    http_resource_params(http_resource_params&& other) :
    url(other.url),
    pipe(std::move(other.pipe)) { }
    
    http_resource_params& operator=(http_resource_params&&) = delete;
    
};

} // namespace
}


#endif	/* STATICLIB_HTTPCLIENT_HTTP_RESOURCE_PARAMS_HPP */

