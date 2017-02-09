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
 * File:   http_resource.hpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:43 AM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTP_RESOURCE_HPP
#define	STATICLIB_HTTPCLIENT_HTTP_RESOURCE_HPP

#include <ios>
#include <istream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "staticlib/config.hpp"
#include "staticlib/io/unbuffered_streambuf.hpp"
#include "staticlib/pimpl.hpp"

#include "staticlib/httpclient/httpclient_exception.hpp"
#include "staticlib/httpclient/http_request_options.hpp"
#include "staticlib/httpclient/http_resource_info.hpp"

namespace staticlib {
namespace httpclient {

/**
 * Remote HTTP resoure that can be read from as a `Source`
 */
class http_resource : public staticlib::pimpl::pimpl_object {
friend class http_session;    
protected:
    /**
     * implementation class
     */
    class impl;

public:
    /**
     * PIMPL-specific constructor
     * 
     * @param pimpl impl object
     */
    PIMPL_CONSTRUCTOR(http_resource)

    /**
     * Reads some data from a remote resource
     * 
     * @param span output span
     * @return number of bytes processed
     */
    std::streamsize read(staticlib::config::span<char> span);
    
    /**
     * Accessor for the resource metainformation
     * 
     * @return 
     */
    const http_resource_info& get_info() const;

private:
    /**
     * Private constructor for implementation details
     * 
     * @param multi_handle implementation defined
     * @param url target HTTP URL
     * @param post_data data to upload
     * @param options request options
     */
    http_resource(/* CURLM */ void* multi_handle,
            std::string url,
            std::unique_ptr<std::istream> post_data,
            http_request_options options);
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTP_RESOURCE_HPP */

