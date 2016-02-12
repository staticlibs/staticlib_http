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

#include <memory>
#include <sstream>
#include <streambuf>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/pimpl.hpp"

#include "staticlib/httpclient/HttpClientException.hpp"
#include "staticlib/httpclient/HttpResource.hpp"
#include "staticlib/httpclient/HttpRequestOptions.hpp"
#include "staticlib/httpclient/HttpSessionOptions.hpp"

namespace staticlib {
namespace httpclient {

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
    
    HttpSession(HttpSessionOptions options = HttpSessionOptions{});

    HttpResource open_url(std::string url,
            HttpRequestOptions options = HttpRequestOptions{});
    
    HttpResource open_url(std::string url, 
            std::streambuf* post_data = nullptr,
            HttpRequestOptions options = HttpRequestOptions{});

    HttpResource open_url(std::string url, 
            std::unique_ptr<std::streambuf> post_data,
            HttpRequestOptions options = HttpRequestOptions{});

    template<typename PostDataSource>
    HttpResource open_url(std::string url,
            PostDataSource&& post_data,
            HttpRequestOptions options = HttpRequestOptions{}) {
        std::unique_ptr<std::streambuf> sbuf{
            staticlib::io::make_unbuffered_istreambuf_ptr(std::forward(post_data))};
        return open_url(std::move(url), std::move(sbuf), std::move(options));
    }
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTPSESSION_HPP */

