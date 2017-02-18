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
 * File:   curl_headers.hpp
 * Author: alex
 *
 * Created on February 13, 2017, 7:01 PM
 */

#ifndef STATICLIB_HTTPCLIENT_CURL_HEADERS_HPP
#define	STATICLIB_HTTPCLIENT_CURL_HEADERS_HPP

#include <memory>
#include <string>
#include <vector>

#include "curl/curl.h"

#include "staticlib/config.hpp"

#include "staticlib/httpclient/http_request_options.hpp"

#include "curl_deleters.hpp"

namespace staticlib {
namespace httpclient {

class curl_headers {
    std::vector<std::string> stored_headers;
    std::unique_ptr<struct curl_slist, curl_slist_deleter> slist;

public:
    curl_headers() { }   
    
    curl_headers(const curl_headers&) = delete;

    curl_headers& operator=(const curl_headers&) = delete;

    staticlib::config::optional<curl_slist*> wrap_into_slist(
            const std::vector<std::pair<std::string, std::string>>& provided_headers) {
        namespace sc = staticlib::config;
        for (auto& pa : provided_headers) {
            stored_headers.emplace_back(pa.first + ": " + pa.second);
            curl_slist* released = slist.release();
            curl_slist* ptr = curl_slist_append(released, stored_headers.back().c_str());
            if (nullptr == ptr) throw httpclient_exception(TRACEMSG(
                    "Error appending header, key: [" + pa.first + "]," +
                    " value: [" + pa.second + "]," +
                    " appended count: [" + sc::to_string(stored_headers.size()) + "]"));
            slist.reset(ptr);
        }
        if (nullptr != slist.get()) {
            return sc::make_optional(slist.get());
        }
        return sc::optional<curl_slist*>();
    }
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_CURL_HEADERS_HPP */

