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
 * File:   HttpSession.hpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:42 AM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTPSESSION_HPP
#define	STATICLIB_HTTPCLIENT_HTTPSESSION_HPP

#include <istream>
#include <memory>
#include <sstream>
#include <streambuf>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef STATICLIB_WITH_ICU
#include <unicode/unistr.h>
#else
#include <string>
#endif // STATICLIB_WITH_ICU

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/pimpl.hpp"

#include "staticlib/httpclient/HttpClientException.hpp"
#include "staticlib/httpclient/HttpResource.hpp"
#include "staticlib/httpclient/HttpRequestOptions.hpp"
#include "staticlib/httpclient/HttpSessionOptions.hpp"

namespace staticlib {
namespace httpclient {

/**
 * Context object for one or more HTTP requests. Caches TCP connections.
 */
class HttpSession : public staticlib::pimpl::PimplObject {
    /**
     * Implementation class
     */
    class Impl;
    
public:   
    /**
     * PIMPL-specific constructor
     * 
     * @param pimpl impl object
     */
    PIMPL_CONSTRUCTOR(HttpSession)
    
    /**
     * Constructor
     * 
     * @param options session options
     */
    HttpSession(HttpSessionOptions options = HttpSessionOptions{});

    /**
     * Opens specified HTTP url as a Source
     * 
     * @param url HTTP URL
     * @param options request options
     * @return HTTP resource
     */
    HttpResource open_url(
#ifdef STATICLIB_WITH_ICU            
            icu::UnicodeString url,
#else
            std::string url,
#endif // STATICLIB_WITH_ICU
            HttpRequestOptions options = HttpRequestOptions{});
    
    /**
     * Opens specified HTTP url as a Source using POST method
     * 
     * @param url HTTP URL
     * @param post_data data to upload
     * @param options request options
     * @return HTTP resource
     */
    HttpResource open_url(
#ifdef STATICLIB_WITH_ICU            
            icu::UnicodeString url,
#else
            std::string url,
#endif // STATICLIB_WITH_ICU
            std::streambuf* post_data = nullptr,
            HttpRequestOptions options = HttpRequestOptions{});

    /**
     * Opens specified HTTP url as a Source using POST method
     * 
     * @param url HTTP URL
     * @param post_data data to upload
     * @param options request options
     * @return HTTP resource
     */
    HttpResource open_url(
#ifdef STATICLIB_WITH_ICU            
            icu::UnicodeString url,
#else
            std::string url,
#endif // STATICLIB_WITH_ICU 
            std::unique_ptr<std::istream> post_data,
            HttpRequestOptions options = HttpRequestOptions{});

    /**
     * Opens specified HTTP url as a Source using POST method
     * 
     * @param url HTTP URL
     * @param post_data data to upload
     * @param options request options
     * @return HTTP resource
     */
    template<typename PostDataSource,
            class = typename std::enable_if<!std::is_lvalue_reference<PostDataSource>::value>::type>
    HttpResource open_url(
#ifdef STATICLIB_WITH_ICU            
            icu::UnicodeString url,
#else
            std::string url,
#endif // STATICLIB_WITH_ICU
            PostDataSource&& post_data,
            HttpRequestOptions options = HttpRequestOptions{}) {
        std::unique_ptr<std::istream> sbuf{
            staticlib::io::make_source_istream_ptr(std::move(post_data))};
        return open_url(std::move(url), std::move(sbuf), std::move(options));
    }

    /**
     * Opens specified HTTP url as a Source using POST method
     * 
     * @param url HTTP URL
     * @param post_data data to upload
     * @param options request options
     * @return HTTP resource
     */            
    template<typename PostDataSource>
    HttpResource open_url(
#ifdef STATICLIB_WITH_ICU            
            icu::UnicodeString url,
#else
            std::string url,
#endif // STATICLIB_WITH_ICU
            PostDataSource& post_data,
            HttpRequestOptions options = HttpRequestOptions{}) {
        std::unique_ptr<std::istream> sbuf{
            staticlib::io::make_source_istream_ptr(post_data)};
        return open_url(std::move(url), std::move(sbuf), std::move(options));
    }            
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTPSESSION_HPP */

