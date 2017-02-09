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
 * File:   http_session.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:42 AM
 */

#include "staticlib/httpclient/http_session.hpp"

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

class http_session::impl : public staticlib::pimpl::pimpl_object::impl {
    std::unique_ptr<CURLM, CurlMultiDeleter> handle;
    std::istringstream empty_stream;

public:
    impl(http_session_options options) :
    handle(curl_multi_init(), CurlMultiDeleter{}) { 
        if (nullptr == handle.get()) throw httpclient_exception(TRACEMSG("Error initializing cURL multi handle"));
//        available since 7.30.0, todo: curl version checking
//        setopt_uint32(CURLMOPT_MAX_HOST_CONNECTIONS, options.max_host_connections);
//        setopt_uint32(CURLMOPT_MAX_TOTAL_CONNECTIONS, options.max_total_connections);
        setopt_uint32(CURLMOPT_MAXCONNECTS, options.maxconnects);
    }
    
    ~impl() STATICLIB_NOEXCEPT { }

    http_resource open_url(http_session& frontend,
            std::string url,
            http_request_options options) {
        if ("" == options.method) {
            options.method = "GET";
        }
        return open_url(frontend, std::move(url), nullptr, std::move(options));
    }
    
    http_resource open_url(http_session& frontend,
            std::string url,
            std::streambuf* post_data, http_request_options options) {
        auto sbuf_ptr = nullptr != post_data ? post_data : EMPTY_STREAM.rdbuf();
        if ("" == options.method) {
            options.method = "POST";
        }
        std::unique_ptr<std::istream> sbuf{
                io::make_source_istream_ptr(io::streambuf_source(sbuf_ptr))};
        return open_url(frontend, std::move(url), std::move(sbuf), std::move(options));
    }
    
    http_resource open_url(http_session&,
            std::string url,
            std::unique_ptr<std::istream> post_data, http_request_options options) {
        if ("" == options.method) {
            options.method = "POST";
        }
        return http_resource(handle.get(), std::move(url), std::move(post_data), std::move(options));
    }
    
private:
    void setopt_uint32(CURLMoption opt, uint32_t value) {
        if (0 == value) return;
        CURLMcode err = curl_multi_setopt(handle.get(), opt, value);
        if (err != CURLM_OK) throw httpclient_exception(TRACEMSG(
                "Error setting session option: [" + sc::to_string(opt) + "]," +
                " to value: [" + sc::to_string(value) + "]"));
    }
    
};
PIMPL_FORWARD_CONSTRUCTOR(http_session, (http_session_options), (), httpclient_exception)
PIMPL_FORWARD_METHOD(http_session, http_resource, open_url, (std::string)(http_request_options), (), httpclient_exception)
PIMPL_FORWARD_METHOD(http_session, http_resource, open_url, (std::string)(std::streambuf*)(http_request_options), (), httpclient_exception)
PIMPL_FORWARD_METHOD(http_session, http_resource, open_url, (std::string)(std::unique_ptr<std::istream>)(http_request_options), (), httpclient_exception)        

} // namespace
}
