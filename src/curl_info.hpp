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
 * File:   curl_info.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 4:03 PM
 */

#ifndef STATICLIB_HTTP_CURL_INFO_HPP
#define	STATICLIB_HTTP_CURL_INFO_HPP

#include <cstdint>
#include <cstdint>

#include "curl/curl.h"

#include "staticlib/config.hpp"

#include "staticlib/http/http_exception.hpp"

namespace staticlib {
namespace http {

class curl_info {
    CURL* handle;
       
public:
    curl_info(CURL* handle):
    handle(handle) { }

    curl_info(const curl_info&) = delete;

    curl_info& operator=(const curl_info&) = delete;
    
    long getinfo_long(CURLINFO opt) {
        namespace sc = staticlib::config;
        long out = -1;
        CURLcode err = curl_easy_getinfo(handle, opt, std::addressof(out));
        if (err != CURLE_OK) throw http_exception(TRACEMSG(
                "cURL curl_easy_getinfo error: [" + curl_easy_strerror(err) + "]," +
                " option: [" + sl::support::to_string(opt) + "]"));
        return out;
    }

    double getinfo_double(CURLINFO opt) {
        namespace sc = staticlib::config;
        double out = -1;
        CURLcode err = curl_easy_getinfo(handle, opt, std::addressof(out));
        if (err != CURLE_OK) throw http_exception(TRACEMSG(
                "cURL curl_easy_getinfo error: [" + curl_easy_strerror(err) + "]," +
                " option: [" + sl::support::to_string(opt) + "]"));
        return out;
    }

    std::string getinfo_string(CURLINFO opt) {
        namespace sc = staticlib::config;
        char* out = nullptr;
        CURLcode err = curl_easy_getinfo(handle, opt, std::addressof(out));
        if (err != CURLE_OK) throw http_exception(TRACEMSG(
                "cURL curl_easy_getinfo error: [" + curl_easy_strerror(err) + "]," +
                " option: [" + sl::support::to_string(opt) + "]"));
        return std::string(out);
    }

};

inline resource_info curl_collect_info(CURL* handle) {
    resource_info info;
    curl_info ci(handle);
    info.effective_url = ci.getinfo_string(CURLINFO_EFFECTIVE_URL);
    info.total_time_secs = ci.getinfo_double(CURLINFO_TOTAL_TIME);
    info.namelookup_time_secs = ci.getinfo_double(CURLINFO_NAMELOOKUP_TIME);
    info.connect_time_secs = ci.getinfo_double(CURLINFO_CONNECT_TIME);
    info.appconnect_time_secs = ci.getinfo_double(CURLINFO_APPCONNECT_TIME);
    info.pretransfer_time_secs = ci.getinfo_double(CURLINFO_PRETRANSFER_TIME);
    info.starttransfer_time_secs = ci.getinfo_double(CURLINFO_STARTTRANSFER_TIME);
    info.redirect_time_secs = ci.getinfo_double(CURLINFO_REDIRECT_TIME);
    info.redirect_count = ci.getinfo_long(CURLINFO_REDIRECT_COUNT);
    info.speed_download_bytes_secs = ci.getinfo_double(CURLINFO_SPEED_DOWNLOAD);
    info.speed_upload_bytes_secs = ci.getinfo_double(CURLINFO_SPEED_UPLOAD);
    info.header_size_bytes = ci.getinfo_long(CURLINFO_HEADER_SIZE);
    info.request_size_bytes = ci.getinfo_long(CURLINFO_REQUEST_SIZE);
    info.ssl_verifyresult = ci.getinfo_long(CURLINFO_SSL_VERIFYRESULT);
    info.os_errno = ci.getinfo_long(CURLINFO_OS_ERRNO);
    info.num_connects = ci.getinfo_long(CURLINFO_NUM_CONNECTS);
    info.primary_ip = ci.getinfo_string(CURLINFO_PRIMARY_IP);
    info.primary_port = ci.getinfo_long(CURLINFO_PRIMARY_PORT);
    return info;
}


} // namespace
}

#endif	/* STATICLIB_HTTP_CURL_INFO_HPP */

