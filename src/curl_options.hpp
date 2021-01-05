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

#ifndef STATICLIB_HTTP_CURL_OPTIONS_HPP
#define STATICLIB_HTTP_CURL_OPTIONS_HPP

#include <cstdint>
#include <memory>
#include <cstdint>

#include "curl/curl.h"

#include "staticlib/config.hpp"

#include "staticlib/http/request_options.hpp"
#include "staticlib/http/session_options.hpp"
#include "staticlib/http/http_exception.hpp"

#include "curl_headers.hpp"

namespace staticlib {
namespace http {

template<typename T>
class curl_options {
    sl::support::observer_ptr<T> cb_obj;
    sl::support::observer_ptr<std::string> url;
    sl::support::observer_ptr<request_options> options;
    sl::support::observer_ptr<std::istream> post_data;
    sl::support::observer_ptr<curl_headers> headers;
    // CURL is void so cannot be used with observer
    CURL* handle;

public:
    curl_options(T* cb_obj, std::string& url, request_options& options,
            std::unique_ptr<std::istream>& post_data, curl_headers& headers,
            std::unique_ptr<CURL, curl_easy_deleter>& handle) :
    cb_obj(sl::support::make_observer_ptr(cb_obj)),
    url(sl::support::make_observer_ptr(url)),
    options(sl::support::make_observer_ptr(options)),
    post_data(sl::support::make_observer_ptr(post_data.get())),
    headers(sl::support::make_observer_ptr(headers)),
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
        if (err_wf != CURLE_OK) throw http_exception(TRACEMSG(
                "Error setting option: [CURLOPT_WRITEFUNCTION], error: [" + curl_easy_strerror(err_wf) + "]"));
        setopt_object(CURLOPT_HEADERDATA, cb_obj.get());
        CURLcode err_hf = curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, curl_options<T>::headers_callback);
        if (err_hf != CURLE_OK) throw http_exception(TRACEMSG(
                "Error setting option: [CURLOPT_HEADERFUNCTION], error: [" + curl_easy_strerror(err_hf) + "]"));

        // general behavior options
        if (options->force_http_10) {
            CURLcode err = curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
            if (err != CURLE_OK) throw http_exception(TRACEMSG(
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
            if (err != CURLE_OK) throw http_exception(TRACEMSG(
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
        if (0 == value) return;
        if (value > static_cast<uint32_t> (std::numeric_limits<int32_t>::max())) throw http_exception(TRACEMSG(
                "Error setting option: [" + sl::support::to_string(opt) + "]," +
                " to invalid overflow value: [" + sl::support::to_string(value) + "]"));
        CURLcode err = curl_easy_setopt(handle, opt, static_cast<long> (value));
        if (err != CURLE_OK) throw http_exception(TRACEMSG(
                "Error setting option: [" + sl::support::to_string(opt) + "]," +
                " to value: [" + sl::support::to_string(value) + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    void setopt_bool(CURLoption opt, bool value) {
        CURLcode err = curl_easy_setopt(handle, opt, value ? 1 : 0);
        if (err != CURLE_OK) throw http_exception(TRACEMSG(
                "Error setting option: [" + sl::support::to_string(opt) + "]," +
                " to value: [" + sl::support::to_string(value) + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    void setopt_string(CURLoption opt, const std::string& value) {
        if ("" == value) return;
        CURLcode err = curl_easy_setopt(handle, opt, value.c_str());
        if (err != CURLE_OK) throw http_exception(TRACEMSG(
                "Error setting option: [" + sl::support::to_string(opt) + "]," +
                " to value: [" + value + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    void setopt_object(CURLoption opt, void* value) {
        if (nullptr == value) return;
        CURLcode err = curl_easy_setopt(handle, opt, value);
        if (err != CURLE_OK) throw http_exception(TRACEMSG(
                "Error setting option: [" + sl::support::to_string(opt) + "]," +
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
        } else throw http_exception(TRACEMSG(
                "Unsupported HTTP method: [" + options->method + "]"));
        if (nullptr != post_data.get() && ("POST" == options->method || "PUT" == options->method)) {
            setopt_object(CURLOPT_READDATA, cb_obj.get());
            CURLcode err_wf = curl_easy_setopt(handle, CURLOPT_READFUNCTION, curl_options<T>::read_callback);
            if (err_wf != CURLE_OK) throw http_exception(TRACEMSG(
                    "Error setting option: [CURLOPT_READFUNCTION], error: [" + curl_easy_strerror(err_wf) + "]"));
            if (options->send_request_body_content_length) {
                CURLcode err = curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, static_cast<long> (options->request_body_content_length));
                if (err != CURLE_OK) throw http_exception(TRACEMSG(
                        "Error setting option: [CURLOPT_POSTFIELDSIZE]," +
                        " to value: [" + sl::support::to_string(options->request_body_content_length) + "]," +
                        " error: [" + curl_easy_strerror(err) + "]"));
            } else {
                options->headers.emplace_back("Transfer-Encoding", "chunked");
            }
        }
    }

};

class curl_multi_options {
    CURLM* handle;
    sl::support::observer_ptr<session_options> options;

public:
    curl_multi_options(CURLM* handle, session_options& options) :
    handle(handle),
    options(sl::support::make_observer_ptr(options)) { }

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
        if (0 == value) return;
        CURLMcode err = curl_multi_setopt(handle, opt, value);
        if (err != CURLM_OK) throw http_exception(TRACEMSG(
                "Error setting session option: [" + sl::support::to_string(opt) + "]," +
                " to value: [" + sl::support::to_string(value) + "]," +
                " error: [" + curl_multi_strerror(err) + "]"));
    }

};

template<typename T>
void apply_curl_options(T* cb_obj, std::string& url, request_options& options,
        std::unique_ptr<std::istream>& post_data, curl_headers& headers,
        std::unique_ptr<CURL, curl_easy_deleter>& handle) {
    curl_options<T>(cb_obj, url, options, post_data, headers, handle).apply();
}

inline void apply_curl_multi_options(CURLM* handle, session_options& options) {
    curl_multi_options(handle, options).apply();
}

} // namespace
}

#endif /* STATICLIB_HTTP_CURL_OPTIONS_HPP */

