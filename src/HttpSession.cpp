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
 * File:   HttpSession.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:42 AM
 */

#include "staticlib/httpclient/HttpSession.hpp"

#include <memory>

#include "curl/curl.h"

#include "staticlib/pimpl/pimpl_forward_macros.hpp"


namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace sc = staticlib::config;
namespace io = staticlib::io;

const std::istringstream EMPTY_STREAM{""};

class CurlMultiDeleter {
public:
    void operator()(CURLM* curlm) {
        curl_multi_cleanup(curlm);
    }   
};

} // namespace

class HttpSession::Impl : public staticlib::pimpl::PimplObject::Impl {
    std::unique_ptr<CURLM, CurlMultiDeleter> handle;
    std::istringstream empty_stream;

public:
    Impl(HttpSessionOptions options) :
    handle(curl_multi_init(), CurlMultiDeleter{}) { 
        if (nullptr == handle.get()) throw HttpClientException(TRACEMSG("Error initializing cURL multi handle"));
//        available since 7.30.0, todo: curl version checking
//        setopt_uint32(CURLMOPT_MAX_HOST_CONNECTIONS, options.max_host_connections);
//        setopt_uint32(CURLMOPT_MAX_TOTAL_CONNECTIONS, options.max_total_connections);
        setopt_uint32(CURLMOPT_MAXCONNECTS, options.maxconnects);
    }
    
    ~Impl() STATICLIB_NOEXCEPT { }

    HttpResource open_url(HttpSession& frontend,
#ifdef STATICLIB_WITH_ICU
            icu::UnicodeString url,
#else
            std::string url,
#endif // STATICLIB_WITH_ICU
            HttpRequestOptions options) {
        return open_url(frontend, std::move(url), nullptr, std::move(options));
    }
    
    HttpResource open_url(HttpSession& frontend,
#ifdef STATICLIB_WITH_ICU
            icu::UnicodeString url,
#else
            std::string url,
#endif // STATICLIB_WITH_ICU
            std::streambuf* post_data, HttpRequestOptions options) {
        auto sbuf_ptr = nullptr != post_data ? post_data : EMPTY_STREAM.rdbuf();
        if ("" == options.method) {
            options.method = "POST";
        }
        std::unique_ptr<std::istream> sbuf{
                io::make_source_istream_ptr(io::streambuf_source(sbuf_ptr))};
        return open_url(frontend, std::move(url), std::move(sbuf), std::move(options));
    }
    
    HttpResource open_url(HttpSession&,
#ifdef STATICLIB_WITH_ICU
            icu::UnicodeString url,
#else
            std::string url,
#endif // STATICLIB_WITH_ICU
            std::unique_ptr<std::istream> post_data, HttpRequestOptions options) {
        if ("" == options.method) {
            options.method = "POST";
        }
        return HttpResource(handle.get(), std::move(url), std::move(post_data), std::move(options));
    }
    
private:
    void setopt_uint32(CURLMoption opt, uint32_t value) {
        if (0 == value) return;
        CURLMcode err = curl_multi_setopt(handle.get(), opt, value);
        if (err != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() +
                "Error setting session option: [" + sc::to_string(opt) + "]," +
                " to value: [" + sc::to_string(value) + "]"));
    }
    
};
PIMPL_FORWARD_CONSTRUCTOR(HttpSession, (HttpSessionOptions), (), HttpClientException)
#ifdef STATICLIB_WITH_ICU
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_url, (icu::UnicodeString)(HttpRequestOptions), (), HttpClientException)
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_url, (icu::UnicodeString)(std::streambuf*)(HttpRequestOptions), (), HttpClientException)
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_url, (icu::UnicodeString)(std::unique_ptr<std::istream>)(HttpRequestOptions), (), HttpClientException)        
#else
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_url, (std::string)(HttpRequestOptions), (), HttpClientException)
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_url, (std::string)(std::streambuf*)(HttpRequestOptions), (), HttpClientException)
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_url, (std::string)(std::unique_ptr<std::istream>)(HttpRequestOptions), (), HttpClientException)        
#endif // STATICLIB_WITH_ICU

} // namespace
}
