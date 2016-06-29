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
#include <istream>
#include <memory>
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

/**
 * Remote HTTP resoure that can be read from as a `Source`
 */
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

    /**
     * Reads some data from a remote resource
     * 
     * @param buffer output buffer
     * @param length number of bytes to process
     * @return number of bytes processed
     */
    std::streamsize read(char* buffer, std::streamsize length);
    
    /**
     * Accessor for the resource metainformation
     * 
     * @return 
     */
    const HttpResourceInfo& get_info() const;

private:
    /**
     * Private constructor for implementation details
     * 
     * @param multi_handle implementation defined
     * @param url target HTTP URL
     * @param post_data data to upload
     * @param options request options
     */
    HttpResource(/* CURLM */ void* multi_handle,
            std::string url,
            std::unique_ptr<std::istream> post_data,
            HttpRequestOptions options);
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTPRESOURCE_HPP */

