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
 * File:   curl_utils.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 8:29 PM
 */

#ifndef STATICLIB_HTTP_CURL_UTILS_HPP
#define STATICLIB_HTTP_CURL_UTILS_HPP

#include <cstring>
#include <string>
#include <utility>

#include "staticlib/support.hpp"

namespace staticlib {
namespace http {

// https://curl.haxx.se/mail/lib-2007-07/0339.html
// https://stackoverflow.com/q/5852784/314015
inline struct timeval create_timeout_struct(long timeo, uint16_t max_timeout_millis) {
    uint16_t tm_u16 = max_timeout_millis;
    if (sl::support::is_uint16_positive(timeo)) {
        uint16_t timeo_u16 = static_cast<uint16_t>(timeo);
        if (timeo_u16 < max_timeout_millis) {
            tm_u16 = timeo_u16;
        }
    }
    long tm_long = static_cast<long> (tm_u16);
    
    struct timeval timeout;
    std::memset(std::addressof(timeout), '\0', sizeof (timeout));
    timeout.tv_sec = tm_long / 1000;
    timeout.tv_usec = (tm_long % 1000) * 1000;
    return timeout;
}

inline fd_set create_fd() {
    fd_set res;
    FD_ZERO(std::addressof(res));
    return res;
}

// http://stackoverflow.com/a/9681122/314015
inline sl::support::optional<std::pair<std::string, std::string>> curl_parse_header(const char* buffer, size_t len) {
    std::string name;
    std::string value;
    size_t i = 0;
    // 2 for '\r\n'
    for (; i < len - 2; i++) {
        if (':' != buffer[i]) {
            name.push_back(buffer[i]);
        } else {
            break;
        }
    }
    if (':' == buffer[i]) {
        // 2 for ': ', 2 for '\r\n'
        size_t valen = len - i - 2 - 2;
        if (valen > 0) {
            value.resize(valen);
            std::memcpy(std::addressof(value.front()), buffer + i + 2, value.length());
            return sl::support::make_optional(std::make_pair(std::move(name), std::move(value)));
        }
    }
    return sl::support::optional<std::pair<std::string, std::string>>();
}

} // namespace
}

#endif /* STATICLIB_HTTP_CURL_UTILS_HPP */

