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
 * File:   curl_options->hpp
 * Author: alex
 *
 * Created on March 24, 2017, 1:02 PM
 */

#ifndef STATICLIB_HTTPCLIENT_CURL_OPTIONS_HPP
#define	STATICLIB_HTTPCLIENT_CURL_OPTIONS_HPP

#include <cstdint>
#include <memory>
#include <cstdint>

#include "curl/curl.h"

#include "staticlib/config.hpp"

#include "staticlib/httpclient/http_request_options.hpp"
#include "staticlib/httpclient/http_session_options.hpp"
#include "staticlib/httpclient/httpclient_exception.hpp"

#include "curl_headers.hpp"

namespace staticlib {
namespace httpclient {

template<typename T>
class curl_options {
    staticlib::config::observer_ptr<T> cb_obj;
    staticlib::config::observer_ptr<std::string> url;
    staticlib::config::observer_ptr<http_request_options> options;
    staticlib::config::observer_ptr<std::istream> post_data;
    staticlib::config::observer_ptr<curl_headers> headers;
    // CURL is void so cannot be used with observer
    CURL* handle;
    
public:
    curl_options(T* cb_obj, std::string& url, http_request_options& options,
            std::unique_ptr<std::istream>& post_data, curl_headers& headers,
            std::unique_ptr<CURL, curl_easy_deleter>& handle) :
    cb_obj(staticlib::config::make_observer_ptr(cb_obj)),
    url(staticlib::config::make_observer_ptr(url)),
    options(staticlib::config::make_observer_ptr(options)),
    post_data(staticlib::config::make_observer_ptr(post_data.get())),
    headers(staticlib::config::make_observer_ptr(headers)),
    handle(handle.get()) { }
    
    curl_options(const curl_options&) = delete;
    
    curl_options& operator=(const curl_options&) = delete;
    
    void apply() {
        // url
        setopt_string(CURLOPT_URL, *url);

        // method
        appply_method();

        // headers
        auto slist = headers->wrap_into_slist(this->options->headers);
        if (slist.has_value()) {
            setopt_object(CURLOPT_HTTPHEADER, static_cast<void*> (slist.value()));
        }

        // callbacks
        setopt_object(CURLOPT_WRITEDATA, cb_obj.get());
        CURLcode err_wf = curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_options<T>::write_callback);
        if (err_wf != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [CURLOPT_WRITEFUNCTION], error: [" + curl_easy_strerror(err_wf) + "]"));
        setopt_object(CURLOPT_HEADERDATA, cb_obj.get());
        CURLcode err_hf = curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, curl_options<T>::headers_callback);
        if (err_hf != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [CURLOPT_HEADERFUNCTION], error: [" + curl_easy_strerror(err_hf) + "]"));

        // general behavior options
        if (options->force_http_10) {
            CURLcode err = curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
            if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                    "Error setting option: [CURLOPT_HTTP_VERSION], error: [" + curl_easy_strerror(err) + "]"));
        }
        setopt_bool(CURLOPT_NOPROGRESS, options->noprogress);
        setopt_bool(CURLOPT_NOSIGNAL, options->nosignal);
        setopt_bool(CURLOPT_FAILONERROR, options->failonerror);
        // Aded in 7.42.0 
#if LIBCURL_VERSION_NUM >= 0x072a00
        setopt_bool(CURLOPT_PATH_AS_IS, options->path_as_is);
#endif // LIBCURL_VERSION_NUM

        // TCP options
        setopt_bool(CURLOPT_TCP_NODELAY, options->tcp_nodelay);
        setopt_bool(CURLOPT_TCP_KEEPALIVE, options->tcp_keepalive);
        setopt_uint32(CURLOPT_TCP_KEEPIDLE, options->tcp_keepidle_secs);
        setopt_uint32(CURLOPT_TCP_KEEPINTVL, options->tcp_keepintvl_secs);
        setopt_uint32(CURLOPT_CONNECTTIMEOUT_MS, options->connecttimeout_millis);
        setopt_uint32(CURLOPT_TIMEOUT_MS, options->timeout_millis);

        // HTTP options
        setopt_uint32(CURLOPT_BUFFERSIZE, options->buffersize_bytes);
        setopt_string(CURLOPT_ACCEPT_ENCODING, options->accept_encoding);
        setopt_bool(CURLOPT_FOLLOWLOCATION, options->followlocation);
        setopt_uint32(CURLOPT_MAXREDIRS, options->maxredirs);
        setopt_string(CURLOPT_USERAGENT, options->useragent);

        // throttling options
        setopt_uint32(CURLOPT_MAX_SEND_SPEED_LARGE, options->max_sent_speed_large_bytes_per_second);
        setopt_uint32(CURLOPT_MAX_RECV_SPEED_LARGE, options->max_recv_speed_large_bytes_per_second);

        // SSL options
        setopt_string(CURLOPT_SSLCERT, options->sslcert_filename);
        setopt_string(CURLOPT_SSLCERTTYPE, options->sslcertype);
        setopt_string(CURLOPT_SSLKEY, options->sslkey_filename);
        setopt_string(CURLOPT_SSLKEYTYPE, options->ssl_key_type);
        setopt_string(CURLOPT_KEYPASSWD, options->ssl_keypasswd);
        if (options->require_tls) {
            CURLcode err = curl_easy_setopt(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
            if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                    "Error setting option: [CURLOPT_SSLVERSION], error: [" + curl_easy_strerror(err) + "]"));
        }
        if (options->ssl_verifyhost) {
            setopt_uint32(CURLOPT_SSL_VERIFYHOST, 2);
        } else {
            setopt_bool(CURLOPT_SSL_VERIFYHOST, false);
        }
        setopt_bool(CURLOPT_SSL_VERIFYPEER, options->ssl_verifypeer);
        // Added in 7.41.0
#if LIBCURL_VERSION_NUM >= 0x072900        
        setopt_bool(CURLOPT_SSL_VERIFYSTATUS, options->ssl_verifystatus);
#endif // LIBCURL_VERSION_NUM        
        setopt_string(CURLOPT_CAINFO, options->cainfo_filename);
        setopt_string(CURLOPT_CRLFILE, options->crlfile_filename);
        setopt_string(CURLOPT_SSL_CIPHER_LIST, options->ssl_cipher_list);
    }

    static size_t headers_callback(char* buffer, size_t size, size_t nitems, void* userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t> (-1);
        T* ptr = static_cast<T*> (userp);
        try {
            return ptr->write_headers(buffer, size, nitems);
        } catch (const std::exception& e) {
            ptr->append_error(TRACEMSG(e.what()));
            return 0;
        }
    }

    static size_t write_callback(char* buffer, size_t size, size_t nitems, void* userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t> (-1);
        T* ptr = static_cast<T*> (userp);
        try {
            return ptr->write_data(buffer, size, nitems);
        } catch (const std::exception& e) {
            ptr->append_error(TRACEMSG(e.what()));
            return 0;
        }
    }

    static size_t read_callback(char *buffer, size_t size, size_t nitems, void *userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t> (-1);
        T* ptr = static_cast<T*> (userp);
        try {
            return ptr->read_data(buffer, size, nitems);
        } catch (const std::exception& e) {
            ptr->append_error(TRACEMSG(e.what()));
            return CURL_READFUNC_ABORT;
        }
    }
    
private:

    // note: think about integer overflow 
    void setopt_uint32(CURLoption opt, uint32_t value) {
        namespace sc = staticlib::config;
        if (0 == value) return;
        if (value > static_cast<uint32_t> (std::numeric_limits<int32_t>::max())) throw httpclient_exception(TRACEMSG(
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to invalid overflow value: [" + sc::to_string(value) + "]"));
        CURLcode err = curl_easy_setopt(handle, opt, static_cast<long> (value));
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to value: [" + sc::to_string(value) + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    void setopt_bool(CURLoption opt, bool value) {
        namespace sc = staticlib::config;
        CURLcode err = curl_easy_setopt(handle, opt, value ? 1 : 0);
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to value: [" + sc::to_string(value) + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    void setopt_string(CURLoption opt, const std::string& value) {
        namespace sc = staticlib::config;
        if ("" == value) return;
        CURLcode err = curl_easy_setopt(handle, opt, value.c_str());
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to value: [" + value + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    void setopt_object(CURLoption opt, void* value) {
        namespace sc = staticlib::config;
        if (nullptr == value) return;
        CURLcode err = curl_easy_setopt(handle, opt, value);
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    void appply_method() {
        if ("" == options->method) return;
        if ("GET" == options->method) {
            setopt_bool(CURLOPT_HTTPGET, true);
        } else if ("POST" == options->method) {
            setopt_bool(CURLOPT_POST, true);
        } else if ("PUT" == options->method) {
            setopt_bool(CURLOPT_PUT, true);
        } else if ("DELETE" == options->method) {
            setopt_string(CURLOPT_CUSTOMREQUEST, "DELETE");
        } else throw httpclient_exception(TRACEMSG(
                "Unsupported HTTP method: [" + options->method + "]"));
        if (nullptr != post_data.get() && ("POST" == options->method || "PUT" == options->method)) {
            setopt_object(CURLOPT_READDATA, cb_obj.get());
            CURLcode err_wf = curl_easy_setopt(handle, CURLOPT_READFUNCTION, curl_options<T>::read_callback);
            if (err_wf != CURLE_OK) throw httpclient_exception(TRACEMSG(
                    "Error setting option: [CURLOPT_READFUNCTION], error: [" + curl_easy_strerror(err_wf) + "]"));
            options->headers.emplace_back("Transfer-Encoding", "chunked");
        }
    }   

};

class curl_multi_options {
    staticlib::config::observer_ptr<http_session_options> options;
    CURLM* handle;
public:
    curl_multi_options(http_session_options& options, std::unique_ptr<CURLM, curl_multi_deleter>& handle) :
    options(staticlib::config::make_observer_ptr(options)),
    handle(handle.get()) { }

    curl_multi_options(const curl_multi_options&) = delete;

    curl_multi_options& operator=(const curl_multi_options&) = delete;

    void apply() {
        // available since 7.30.0        
#if LIBCURL_VERSION_NUM >= 0x071e00
        setopt_uint32(CURLMOPT_MAX_HOST_CONNECTIONS, options->max_host_connections);
        setopt_uint32(CURLMOPT_MAX_TOTAL_CONNECTIONS, options->max_total_connections);
#endif // LIBCURL_VERSION_NUM
        setopt_uint32(CURLMOPT_MAXCONNECTS, options->maxconnects);
    }

private:
    void setopt_uint32(CURLMoption opt, uint32_t value) {
        namespace sc = staticlib::config;
        if (0 == value) return;
        CURLMcode err = curl_multi_setopt(handle, opt, value);
        if (err != CURLM_OK) throw httpclient_exception(TRACEMSG(
                "Error setting session option: [" + sc::to_string(opt) + "]," +
                " to value: [" + sc::to_string(value) + "]," +
                " error: [" + curl_multi_strerror(err) + "]"));
    }        
    
};

template<typename T>
void apply_curl_options(T* cb_obj, std::string& url, http_request_options& options,
        std::unique_ptr<std::istream>& post_data, curl_headers& headers,
        std::unique_ptr<CURL, curl_easy_deleter>& handle) {
    curl_options<T>(cb_obj, url, options, post_data, headers, handle).apply();
}

inline void apply_curl_multi_options(http_session_options& options, std::unique_ptr<CURLM, curl_multi_deleter>& handle) {
    curl_multi_options(options, handle).apply();
}

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_CURL_OPTIONS_HPP */

