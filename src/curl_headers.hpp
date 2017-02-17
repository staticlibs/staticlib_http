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
    std::vector<std::string> headers;
    std::unique_ptr<struct curl_slist, curl_slist_deleter> slist;

public:
    curl_headers(const http_request_options& options) {
        namespace sc = staticlib::config;
        if (options.headers.empty()) return;
//        struct curl_slist* slist_ptr = nullptr;
        for (auto& pa : options.headers) {
            headers.emplace_back(pa.first + ": " + pa.second);
            curl_slist* released = slist.release();
            curl_slist* ptr = curl_slist_append(released, headers.back().c_str());
            if (nullptr == ptr) throw httpclient_exception(TRACEMSG(
                    "Error appending header, key: [" + pa.first + "]," +
                    " value: [" + pa.second + "]," +
                    " appended count: [" + sc::to_string(headers.size()) + "]"));
            slist.reset(ptr);
        }
    }
    
    curl_headers(const curl_headers&) = delete;

    curl_headers& operator=(const curl_headers&) = delete;

    staticlib::config::optional<curl_slist*> get_curl_slist() {
        namespace sc = staticlib::config;
        if (nullptr != slist.get()) {
            return sc::make_optional(slist.get());
        }
        return sc::optional<curl_slist*>();
    }
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_CURL_HEADERS_HPP */

