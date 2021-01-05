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
 * File:   session.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 5:29 PM
 */

#ifndef STATICLIB_HTTP_SESSION_HPP
#define STATICLIB_HTTP_SESSION_HPP

#include <istream>
#include <memory>
#include <streambuf>

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/pimpl.hpp"

#include "staticlib/http/http_exception.hpp"
#include "staticlib/http/resource.hpp"
#include "staticlib/http/request_options.hpp"
#include "staticlib/http/session_options.hpp"

namespace staticlib {
namespace http {

class session : public sl::pimpl::object {
protected:
    /**
     * Implementation class
     */
    class impl;

    /**
     * Constructor for inheritors
     * 
     * @param options session options
     */
    session(session_options opts = session_options{});

public:
    /**
     * PIMPL-specific constructor
     * 
     * @param pimpl impl object
     */
    PIMPL_CONSTRUCTOR(session)

    /**
     * Opens specified HTTP url as a Source
     * 
     * @param url HTTP URL
     * @param opts request options
     * @return HTTP resource
     */
    resource open_url(
            const std::string& url,
            request_options opts = request_options{});

    /**
     * Opens specified HTTP url as a Source using POST method
     * 
     * @param url HTTP URL
     * @param post_data data to upload
     * @param opts request options
     * @return HTTP resource
     */
    resource open_url(
            const std::string& url,
            std::streambuf* post_data,
            request_options opts = request_options{});

    /**
     * Opens specified HTTP url as a Source using POST method
     * 
     * @param url HTTP URL
     * @param post_data data to upload
     * @param opts request options
     * @return HTTP resource
     */
    template<typename PostDataSource,
            class = typename std::enable_if<!std::is_lvalue_reference<PostDataSource>::value>::type>
    resource open_url(
            const std::string& url,
            PostDataSource&& post_data,
            request_options opts = request_options{}) {
        std::unique_ptr<std::istream> sbuf{
            staticlib::io::make_source_istream_ptr(std::move(post_data))
        };
        return open_url(url, std::move(sbuf), std::move(opts));
    }

    /**
     * Opens specified HTTP url as a Source using POST method
     * 
     * @param url HTTP URL
     * @param post_data data to upload
     * @param opts request options
     * @return HTTP resource
     */
    template<typename PostDataSource>
    resource open_url(
            const std::string& url,
            PostDataSource& post_data,
            request_options opts = request_options{}) {
        std::unique_ptr<std::istream> sbuf{
            staticlib::io::make_source_istream_ptr(post_data)
        };
        return open_url(url, std::move(sbuf), std::move(opts));
    }

    /**
     * Opens specified HTTP url as a Source using POST method
     * 
     * @param url HTTP URL
     * @param post_data data to upload
     * @param opts request options
     * @return HTTP resource
     */
    virtual resource open_url(
            const std::string& url,
            std::unique_ptr<std::istream> post_data,
            request_options opts = request_options{}) = 0;

};

} // namespace
}

#endif /* STATICLIB_HTTP_SESSION_HPP */

