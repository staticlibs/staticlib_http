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
 * File:   multi_threaded_http_session.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 5:35 PM
 */

#ifndef STATICLIB_HTTPCLIENT_MULTI_THREADED_HTTP_SESSION_HPP
#define	STATICLIB_HTTPCLIENT_MULTI_THREADED_HTTP_SESSION_HPP

#include "staticlib/httpclient/basic_http_session.hpp"

namespace staticlib {
namespace httpclient {

class multi_threaded_http_session : public basic_http_session {
protected:
    /**
     * Implementation class
     */
    class impl;

public:
    using basic_http_session::open_url;
    
    /**
     * PIMPL-specific constructor
     * 
     * @param pimpl impl object
     */
    PIMPL_INHERIT_CONSTRUCTOR(multi_threaded_http_session, basic_http_session)

    /**
     * Constructor
     * 
     * @param options session options
     */
    multi_threaded_http_session(http_session_options options = http_session_options{});

    /**
     * Opens specified HTTP url as a Source using POST method
     * 
     * @param url HTTP URL
     * @param post_data data to upload
     * @param options request options
     * @return HTTP resource
     */
    virtual http_resource open_url(
            const std::string& url,
            std::unique_ptr<std::istream> post_data,
            http_request_options options = http_request_options{}) override;
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_MULTI_THREADED_HTTP_SESSION_HPP */

