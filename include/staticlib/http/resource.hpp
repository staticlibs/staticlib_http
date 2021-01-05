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
 * File:   resource.hpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:43 AM
 */

#ifndef STATICLIB_HTTP_RESOURCE_HPP
#define STATICLIB_HTTP_RESOURCE_HPP

#include <ios>
#include <istream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "staticlib/config.hpp"
#include "staticlib/io/unbuffered_streambuf.hpp"
#include "staticlib/pimpl.hpp"

#include "staticlib/http/http_exception.hpp"
#include "staticlib/http/request_options.hpp"
#include "staticlib/http/resource_info.hpp"

namespace staticlib {
namespace http {

/**
 * Remote HTTP resource that can be read from as a `Source`
 */
class resource : public sl::pimpl::object {
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
    PIMPL_CONSTRUCTOR(resource)

    /**
     * Reads some data from a remote resource
     * 
     * @param span output span
     * @return number of bytes processed
     */
    virtual std::streamsize read(sl::io::span<char> span);

    /**
     * Returns URL of this resource
     * 
     * @return URL of this resource
     */
    virtual const std::string& get_url() const;

    /**
     * Returns HTTP status code
     * 
     * @return HTTP status code
     */
    virtual uint16_t get_status_code() const;

    /**
     * Accessor for the resource metainformation
     * 
     * @return 
     */
    virtual resource_info get_info() const;

    /**
     * Accessor for received headers
     * 
     * @return received headers
     */
    virtual const std::vector<std::pair<std::string, std::string>>& get_headers() const;

    /**
     * Returns a value for the specified header, received from server.
     * Uses linear search over headers array.
     * 
     * @param name header name
     * @return header value, empty string if specified header not found
     */
    virtual const std::string& get_header(const std::string& name) const;

    /**
     * Inspect connection attempt results
     * 
     * @return 'true' if connection has been established successfully and
     *         response code has been received, 'false' otherwise
     */
    virtual bool connection_successful() const;
 
};

} // namespace
}

#endif /* STATICLIB_HTTP_RESOURCE_HPP */

