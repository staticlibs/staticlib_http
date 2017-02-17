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
 * File:   http_session_options.hpp
 * Author: alex
 *
 * Created on February 8, 2016, 9:08 PM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTP_SESSION_OPTIONS_HPP
#define	STATICLIB_HTTPCLIENT_HTTP_SESSION_OPTIONS_HPP

#include <cstdint>

namespace staticlib {
namespace httpclient {

/**
 * Configuration options for the HTTP Session
 */
struct http_session_options {
    /**
     * https://curl.haxx.se/libcurl/c/CURLMOPT_MAX_HOST_CONNECTIONS.html
     */
    uint32_t max_host_connections = 0;
    /**
     * https://curl.haxx.se/libcurl/c/CURLMOPT_MAX_TOTAL_CONNECTIONS.html
     */
    uint32_t max_total_connections = 0;
    /**
     * https://curl.haxx.se/libcurl/c/CURLMOPT_MAXCONNECTS.html
     */
    uint32_t maxconnects = 0;
    
    /**
     * Max number of the enqueued requests
     */
    uint32_t requests_queue_max_size = 1024;

    /**
     * Timeout to wait for when reading data unsuccessfully
     */
    uint32_t fdset_timeout_millis = 100;
    
    /**
     * Timeout to wait for when requests queue is not empty, but all 
     * running requests are paused.
     */
    uint32_t all_requests_paused_timeout_millis = 100;
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTP_SESSION_OPTIONS_HPP */

