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
 * File:   HttpSessionOptions.hpp
 * Author: alex
 *
 * Created on February 8, 2016, 9:08 PM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTPSESSIONOPTIONS_HPP
#define	STATICLIB_HTTPCLIENT_HTTPSESSIONOPTIONS_HPP

#include <cstdint>

namespace staticlib {
namespace httpclient {

/**
 * Configuration options for the HTTP Session
 */
struct HttpSessionOptions {
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
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTPSESSIONOPTIONS_HPP */

