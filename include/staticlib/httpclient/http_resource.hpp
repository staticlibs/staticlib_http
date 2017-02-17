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

// forward decl
class http_resource_params;

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
     * Returns URL of this resource
     * 
     * @return URL of this resource
     */
    const std::string& get_url() const;
    
    /**
     * Returns HTTP status code
     * 
     * @return HTTP status code
     */
    uint16_t get_status_code() const;
    
    /**
     * Accessor for the resource metainformation
     * 
     * @return 
     */
    http_resource_info get_info() const;

    /**
     * Accessor for received headers
     * 
     * @return received headers
     */
    const std::vector<std::pair<std::string, std::string>>& get_headers() const;

    /**
     * Returns a value for the specified header, received from server.
     * Uses linear search over headers array.
     * 
     * @param name header name
     * @return header value, empty string if specified header not found
     */
    const std::string& get_header(const std::string& name) const;

    /**
     * Inspect connection attempt results
     * 
     * @return 'true' if connection has been established successfully and
     *         response code has been received, 'false' otherwise
     */
    bool connection_successful() const;
    
private:
    /**
     * Private constructor for implementation details
     * 
     * @param http_resource_params input parameterss
     */
    http_resource(http_resource_params&& params);
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTP_RESOURCE_HPP */

