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
 * File:   http_request_options.hpp
 * Author: alex
 *
 * Created on February 8, 2016, 2:51 PM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTP_REQUEST_OPTIONS_HPP
#define	STATICLIB_HTTPCLIENT_HTTP_REQUEST_OPTIONS_HPP

#include <string>
#include <vector>
#include <utility>
#include <cstdint>

namespace staticlib {
namespace httpclient {

/**
 * Configuration options for the HTTP request
 */
struct http_request_options {
    
    // options implemented manually
    
    /**
     * Headers to send with a request
     */
    std::vector<std::pair<std::string, std::string>> headers;
    /**
     * HTTP method to use
     */
    std::string method = "";
    /**
     * HttpException will be thrown from read method on unsuccessful connection attempt,
     * see also 'connecttimeout_millis'
     */
    bool abort_on_connect_error = true;
    /**
     * HttpException will be thrown from read method on HTTP response code >= 400
     */
    bool abort_on_response_error = true;
    /**
     * Max allowed number of response headers
     */
    uint16_t max_number_of_response_headers = 128;
    /**
     * Consumer thread wakeup timeout (in milliseconds)
     */
    uint16_t consumer_thread_wakeup_timeout_millis = 100;
    
    // general behavior options
    
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_HTTP_VERSION.html
     */
    bool force_http_10 = false;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_NOPROGRESS.html
     */
    bool noprogress = true;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_NOSIGNAL.html
     */
    bool nosignal = true;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_FAILONERROR.html
     */
    bool failonerror = false;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_PATH_AS_IS.html
     */
    bool path_as_is = true;
    
    // TCP options
    
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_TCP_NODELAY.html
     */
    bool tcp_nodelay = false;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_TCP_KEEPALIVE.html
     */
    bool tcp_keepalive = false;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_TCP_KEEPIDLE.html
     */
    uint32_t tcp_keepidle_secs = 300;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_TCP_KEEPINTVL.html
     */
    uint32_t tcp_keepintvl_secs = 300;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_CONNECTTIMEOUT_MS.html
     */
    uint32_t connecttimeout_millis = 10000;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_TIMEOUT_MS.html
     */
    uint32_t timeout_millis = 15000;
    
    // HTTP options
    
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_BUFFERSIZE.html
     */
    uint32_t buffersize_bytes = 16384;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_ACCEPT_ENCODING.html
     */
    std::string accept_encoding = "gzip";
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_FOLLOWLOCATION.html
     */
    bool followlocation = true;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_MAXREDIRS.html
     */
    uint32_t maxredirs = 32;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_USERAGENT.html
     */
    // "Mozilla/5.0 (Linux; U; Android 4.2.2; en-us; GT-I9505 Build/JDQ39) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30"
    std::string useragent = "";
    
    // throttling options
    
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_MAX_SEND_SPEED_LARGE.html
     */
    uint32_t max_sent_speed_large_bytes_per_second = 0;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_MAX_RECV_SPEED_LARGE.html
     */
    uint32_t max_recv_speed_large_bytes_per_second = 0;
    
    // SSL options
    
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_SSLCERT.html
     */
    std::string sslcert_filename = "";
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_SSLCERTTYPE.html
     */
    std::string sslcertype = "";
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_SSLKEY.html
     */
    std::string sslkey_filename = "";
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_SSLKEYTYPE.html
     */
    std::string ssl_key_type = "";
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_KEYPASSWD.html
     */
    std::string ssl_keypasswd = "";
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_SSLVERSION.html
     */
    bool require_tls = false;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_SSL_VERIFYHOST.html
     */
    bool ssl_verifyhost = false;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_SSL_VERIFYPEER.html
     */
    bool ssl_verifypeer = false;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_SSL_VERIFYSTATUS.html
     */
    bool ssl_verifystatus = false;
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_CAINFO.html
     */
    std::string cainfo_filename = "";
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_CRLFILE.html
     */
    std::string crlfile_filename = "";
    /**
     * https://curl.haxx.se/libcurl/c/CURLOPT_SSL_CIPHER_LIST.html
     */
    std::string ssl_cipher_list = "";
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTP_REQUEST_OPTIONS_HPP */

