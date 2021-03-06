/*
 * Copyright 2021, alex at staticlibs.net
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
 * File:   polling_session.hpp
 * Author: alex
 *
 * Created on January 17, 2021, 12:44 AM
 */

#ifndef STATICLIB_HTTP_POLLING_SESSION_HPP
#define STATICLIB_HTTP_POLLING_SESSION_HPP

#include "staticlib/http/session.hpp"

#include <vector>

namespace staticlib {
namespace http {

/**
 * Single-threaded "session" implementation, that can perform multiple requests
 * simultaneously. NOT thread-safe.
 * TCP connections are cached where possible and are bound to the session object.
 */
class polling_session : public session {
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
    PIMPL_INHERIT_CONSTRUCTOR(polling_session, session)

    /**
     * Constructor
     * 
     * @param options session options
     */
    polling_session(session_options options = session_options{});

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

    /**
     * Poll the queue making cURL to actually process the requests
     * 
     * @return list of requests, that finished execution
     */
    std::vector<resource> poll();

    /**
     * Number of request, that were submitted for execution and
     * not yet finished
     * 
     * @return number of enqueued requests
     */
    size_t enqueued_requests_count();
};

} // namespace
}


#endif /* STATICLIB_HTTP_POLLING_SESSION_HPP */

