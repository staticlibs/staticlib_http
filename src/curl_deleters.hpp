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
 * File:   curl_deleters.hpp
 * Author: alex
 *
 * Created on February 13, 2017, 6:53 PM
 */

#ifndef STATICLIB_HTTPCLIENT_CURL_DELETERS_HPP
#define	STATICLIB_HTTPCLIENT_CURL_DELETERS_HPP

#include "curl/curl.h"

namespace staticlib {
namespace httpclient {

class curl_easy_deleter {
    CURLM* multi_handle;

public:
    curl_easy_deleter(CURLM* multi_handle) :
    multi_handle(multi_handle) { }

    void operator()(CURL* curl) {
        curl_multi_remove_handle(multi_handle, curl);
        curl_easy_cleanup(curl);
    }
};


class curl_multi_deleter {
public:
    void operator()(CURLM* curlm) {
        curl_multi_cleanup(curlm);
    }
};


class curl_slist_deleter {
public:
    void operator()(struct curl_slist* list) {
        curl_slist_free_all(list);
    }
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_CURL_DELETERS_HPP */

