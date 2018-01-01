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
 * File:   resource_info.hpp
 * Author: alex
 *
 * Created on February 8, 2016, 2:51 PM
 */

#ifndef STATICLIB_HTTP_RESOURCE_INFO_HPP
#define STATICLIB_HTTP_RESOURCE_INFO_HPP

#include <string>
#include <vector>
#include <utility>
#include <cstdint>

namespace staticlib {
namespace http {

/**
 * Metainformation about the opened HTTP resource
 */
struct resource_info {

    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_EFFECTIVE_URL.html
     */
    std::string effective_url = "";
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_TOTAL_TIME.html
     */
    double total_time_secs = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_NAMELOOKUP_TIME.html
     */
    double namelookup_time_secs = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_CONNECT_TIME.html
     */
    double connect_time_secs = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_APPCONNECT_TIME.html
     */
    double appconnect_time_secs = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_PRETRANSFER_TIME.html
     */
    double pretransfer_time_secs = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_STARTTRANSFER_TIME.html
     */
    double starttransfer_time_secs = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_REDIRECT_TIME.html
     */
    double redirect_time_secs = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_REDIRECT_COUNT.html
     */
    long redirect_count = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_SPEED_DOWNLOAD.html
     */
    double speed_download_bytes_secs = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_SPEED_UPLOAD.html
     */
    double speed_upload_bytes_secs = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_HEADER_SIZE.html
     */
    long header_size_bytes = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_REQUEST_SIZE.html
     */
    long request_size_bytes = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_SSL_VERIFYRESULT.html
     */
    long ssl_verifyresult = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_OS_ERRNO.html
     */
    long os_errno = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_NUM_CONNECTS.html
     */
    long num_connects = -1;
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_PRIMARY_IP.html
     */
    std::string primary_ip = "";
    /**
     * https://curl.haxx.se/libcurl/c/CURLINFO_PRIMARY_PORT.html
     */
    long primary_port = -1;
};

} // namespace
}

#endif /* STATICLIB_HTTP_RESOURCE_INFO_HPP */

