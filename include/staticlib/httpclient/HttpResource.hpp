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
 * File:   HttpResource.hpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:43 AM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTPRESOURCE_HPP
#define	STATICLIB_HTTPCLIENT_HTTPRESOURCE_HPP

#include <ios>
#include <memory>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

#include "staticlib/io/unbuffered_streambuf.hpp"
#include "staticlib/pimpl.hpp"

#include "staticlib/httpclient/HttpClientException.hpp"
#include "staticlib/httpclient/HttpRequestOptions.hpp"
#include "staticlib/httpclient/HttpResourceInfo.hpp"

namespace staticlib {
namespace httpclient {

class HttpResource : public staticlib::pimpl::PimplObject {
friend class HttpSession;    
protected:
    /**
     * Implementation class
     */
    class Impl;

public:
    /**
     * PIMPL-specific constructor
     * 
     * @param pimpl impl object
     */
    PIMPL_CONSTRUCTOR(HttpResource)
            
    std::streamsize read(char* buffer, std::streamsize length);
    
    HttpResourceInfo& get_info();

private:
    /**
     * Private constructor for implementation details
     * 
     * @param impl_ptr implementation defined
     */
    HttpResource(/* CURLM */ void* multi_handle,
            std::string url,
            std::unique_ptr<std::streambuf> post_data,
            HttpRequestOptions options);
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTPRESOURCE_HPP */

