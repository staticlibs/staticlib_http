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
 * File:   multi_threaded_session.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 5:35 PM
 */

#ifndef STATICLIB_HTTP_MULTI_THREADED_SESSION_HPP
#define STATICLIB_HTTP_MULTI_THREADED_SESSION_HPP

#include "staticlib/http/session.hpp"

namespace staticlib {
namespace http {

/**
 * Thread-safe "session" implementation it can be used for performing
 * HTTP queries from multiple threads simultaneously. Separate thread
 * is used to process session operations. TCP connections are
 * cached where possible and are bound to session object.
 */
class multi_threaded_session : public session {
protected:
    /**
     * Implementation class
     */
    class impl;

public:
    using session::open_url;

    /**
     * PIMPL-specific constructor
     * 
     * @param pimpl impl object
     */
    PIMPL_INHERIT_CONSTRUCTOR(multi_threaded_session, session)

    /**
     * Constructor
     * 
     * @param options session options
     */
    multi_threaded_session(session_options options = session_options{});

    /**
     * Opens specified HTTP url as a Source using POST method
     * 
     * @param url HTTP URL
     * @param post_data data to upload
     * @param options request options
     * @return HTTP resource
     */
    virtual resource open_url(
            const std::string& url,
            std::unique_ptr<std::istream> post_data,
            request_options options = request_options{}) override;
};

} // namespace
}

#endif /* STATICLIB_HTTP_MULTI_THREADED_SESSION_HPP */

