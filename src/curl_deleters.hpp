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

