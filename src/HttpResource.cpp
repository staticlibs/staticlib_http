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
 * File:   HttpResource.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:43 AM
 */

#include "staticlib/httpclient/HttpResource.hpp"

#include <chrono>
#include <memory>
#include <thread>
#include <cstring>

#include "curl/curl.h"

#include "staticlib/config.hpp"
#include "staticlib/pimpl/pimpl_forward_macros.hpp"
#include "staticlib/httpclient/HttpResourceInfo.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace sc = staticlib::config;
namespace io = staticlib::io;

typedef const std::vector<std::pair<std::string, std::string>>& headers_type;

class CurlEasyDeleter {
    CURLM* multi_handle;
public:
    CurlEasyDeleter(CURLM* multi_handle) :
    multi_handle(multi_handle) { }
    
    void operator()(CURL* curl) {
        curl_multi_remove_handle(multi_handle, curl);
        curl_easy_cleanup(curl);
    }
};

} // namespace


class HttpResource::Impl : public staticlib::pimpl::PimplObject::Impl {
    CURLM* multi_handle;
    std::unique_ptr<CURL, CurlEasyDeleter> handle;
    std::string url;
    std::unique_ptr<std::streambuf> post_data;
    HttpRequestOptions options;
    
    HttpResourceInfo info;
    std::vector<char> buf;
    size_t buf_idx = 0;
    bool open = false;
    
public:
    Impl(CURLM* multi_handle, 
            std::string url,
            std::unique_ptr<std::streambuf> post_data,
            HttpRequestOptions options) :
    multi_handle(multi_handle),
    handle(curl_easy_init(), CurlEasyDeleter{multi_handle}),
    url(std::move(url)),
    post_data(std::move(post_data)),
    options(std::move(options)) {
        if (nullptr == handle.get()) throw HttpClientException(TRACEMSG("Error initializing cURL handle"));
        CURLMcode errm = curl_multi_add_handle(multi_handle, handle.get());
        if (errm != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() +
                "cURL multi_add error: [" + sc::to_string(errm) + "], url: [" + this->url + "]"));
        curl_easy_setopt(handle.get(), CURLOPT_URL, this->url.c_str());
        curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, this);
        curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, HttpResource::Impl::write_callback);
        curl_easy_setopt(handle.get(), CURLOPT_HEADERDATA, this);
        curl_easy_setopt(handle.get(), CURLOPT_HEADERFUNCTION, HttpResource::Impl::headers_callback);

        curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYHOST, false);
        curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYPEER, false);
        open = true;
    }
    
    virtual ~Impl() STATICLIB_NOEXCEPT { }

    std::streamsize read(HttpResource&, char* buffer, std::streamsize length) {
        size_t ulen = static_cast<size_t> (length);
        fill_buffer();
        if (0 == buf.size()) {
            if (open) {
                return 0;
            } else {
                fill_info();
                return std::char_traits<char>::eof();
            }
        }
        // return from buffer
        size_t avail = buf.size() - buf_idx;
        size_t reslen = avail <= ulen ? avail : ulen;
        std::memcpy(buffer, buf.data(), reslen);
        buf_idx += reslen;
        return static_cast<std::streamsize>(reslen);
    }
    
private:
    static size_t headers_callback(char* buffer, size_t size, size_t nitems, void* userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t>(-1);
        HttpResource::Impl* ptr = static_cast<HttpResource::Impl*> (userp);
        return ptr->write_headers(buffer, size, nitems);
    }

    // http://stackoverflow.com/a/9681122/314015
    size_t write_headers(char *buffer, size_t size, size_t nitems) {
        size_t len = size*nitems;
        std::string name{};
        for (size_t i = 0; i < len; i++) {
            if (':' != buffer[i]) {
                name.push_back(buffer[i]);
            } else {
                std::string value{};
                // 2 for ': ', 2 for '\r\n'
                size_t valen = len - i - 2 - 2;
                if (valen > 0) {
                    value.resize(valen);
                    std::memcpy(std::addressof(value.front()), buffer + i + 2, value.length());
                    info.add_header(std::move(name), std::move(value));
                    break;
                }
            }
        }
        return len;
    }

    static size_t write_callback(char* buffer, size_t size, size_t nitems, void* userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t>(-1);
        HttpResource::Impl* ptr = static_cast<HttpResource::Impl*> (userp);
        return ptr->write_data(buffer, size, nitems);
    }

    size_t write_data(char *buffer, size_t size, size_t nitems) {
        size_t len = size*nitems;
        buf.resize(len);
        std::memcpy(buf.data(), buffer, len);
        return len;
    } 
    
    // curl_multi_socket_action may be used instead of fdset
    void fill_buffer() {
        // some data in buffer
        if (buf_idx < buf.size()) return;
        buf_idx = 0;
        buf.resize(0);
        // attempt to fill buffer
        while (open && 0 == buf.size()) {            
            long timeo = -1;
            CURLMcode err_timeout = curl_multi_timeout(multi_handle, std::addressof(timeo));
            if (err_timeout != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() +
                    "cURL timeout error: [" + sc::to_string(err_timeout) + "], url: [" + url + "]"));
            struct timeval timeout = create_timeout_struct(timeo);

            // get file descriptors from the transfers
            fd_set fdread = create_fd();
            fd_set fdwrite = create_fd();
            fd_set fdexcep = create_fd();
            int maxfd = -1;
            CURLMcode err_fdset = curl_multi_fdset(multi_handle, std::addressof(fdread), 
                    std::addressof(fdwrite), std::addressof(fdexcep), std::addressof(maxfd));
            if (err_fdset != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() + 
                    "cURL fdset error: [" + sc::to_string(err_fdset) + "], url: [" + url + "]"));

            // wait or select
            int err_select = 0;
            if (maxfd == -1) {
                std::this_thread::sleep_for(std::chrono::milliseconds{100});
            } else {
                err_select = select(maxfd + 1, std::addressof(fdread), std::addressof(fdwrite), 
                        std::addressof(fdexcep), std::addressof(timeout));
            }
    
            // do perform if no select error
            if (-1 != err_select) {
                int active = -1;
                CURLMcode err = curl_multi_perform(multi_handle, std::addressof(active));
                if (err != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() +
                        "cURL multi_perform error: [" + sc::to_string(err) + "], url: [" + url + "]"));
                open = (1 == active);
            }
        }
    }

    // http://curl-library.cool.haxx.narkive.com/2sYifbgu/issue-with-curl-multi-timeout-while-doing-non-blocking-http-posts-in-vms
    struct timeval create_timeout_struct(long timeo) {
        struct timeval timeout;
        std::memset(std::addressof(timeout), '\0', sizeof(timeout));
        timeout.tv_sec = 10;
        if (timeo > 0) {
            long ctsecs = timeo / 1000;
            if (ctsecs < 10) {
                timeout.tv_sec = ctsecs;
            }
            timeout.tv_usec = (timeo % 1000) * 1000;
        }
        return timeout;
    }

    fd_set create_fd() {
        fd_set res;
        FD_ZERO(std::addressof(res));
        return res;
    }
    
    long getinfo_long(CURLINFO opt) {
        long out = -1;
        CURLcode err = curl_easy_getinfo(handle.get(), opt, std::addressof(out));
        if (err != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                "cURL curl_easy_getinfo error: [" + sc::to_string(err) + "]," +
                " option: [" + sc::to_string(opt) + "]"));
        return out;
    }
    
    long getinfo_double(CURLINFO opt) {
        double out = -1;
        CURLcode err = curl_easy_getinfo(handle.get(), opt, std::addressof(out));
        if (err != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                "cURL curl_easy_getinfo error: [" + sc::to_string(err) + "]," +
                " option: [" + sc::to_string(opt) + "]"));
        return out;
    }
    
    std::string getinfo_string(CURLINFO opt) {
        char* out = nullptr;
        CURLcode err = curl_easy_getinfo(handle.get(), opt, std::addressof(out));
        if (err != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                "cURL curl_easy_getinfo error: [" + sc::to_string(err) + "]," +
                " option: [" + sc::to_string(opt) + "]"));
        return std::string(out);
    }
    
    void fill_info() {
        if (info.ready) throw HttpClientException(TRACEMSG(std::string() +
                "Resource info is already initialized, url: [" + url + "]"));
        info.effective_url = getinfo_string(CURLINFO_EFFECTIVE_URL);
        info.response_code = getinfo_long(CURLINFO_RESPONSE_CODE);
        info.total_time_secs = getinfo_double(CURLINFO_TOTAL_TIME);
        info.namelookup_time_secs = getinfo_double(CURLINFO_NAMELOOKUP_TIME);
        info.connect_time_secs = getinfo_double(CURLINFO_CONNECT_TIME);
        info.appconnect_time_secs = getinfo_double(CURLINFO_APPCONNECT_TIME);
        info.pretransfer_time_secs = getinfo_double(CURLINFO_PRETRANSFER_TIME);
        info.starttransfer_time_secs = getinfo_double(CURLINFO_STARTTRANSFER_TIME);
        info.redirect_time_secs = getinfo_double(CURLINFO_REDIRECT_TIME);
        info.redirect_count = getinfo_long(CURLINFO_REDIRECT_COUNT);
        info.speed_download_bytes_secs = getinfo_double(CURLINFO_SPEED_DOWNLOAD);
        info.speed_upload_bytes_secs = getinfo_double(CURLINFO_SPEED_UPLOAD);
        info.header_size_bytes = getinfo_long(CURLINFO_HEADER_SIZE);
        info.request_size_bytes = getinfo_long(CURLINFO_REQUEST_SIZE);
        info.ssl_verifyresult = getinfo_long(CURLINFO_SSL_VERIFYRESULT);
        info.os_errno = getinfo_long(CURLINFO_OS_ERRNO);
        info.num_connects = getinfo_long(CURLINFO_NUM_CONNECTS);
        info.primary_ip = getinfo_string(CURLINFO_PRIMARY_IP);
        info.primary_port = getinfo_long(CURLINFO_PRIMARY_PORT);
        info.ready = true;
    }
    
};
PIMPL_FORWARD_CONSTRUCTOR(HttpResource, (CURLM*)(std::string)(std::unique_ptr<std::streambuf>)(HttpRequestOptions), (), HttpClientException)
PIMPL_FORWARD_METHOD(HttpResource, std::streamsize, read, (char*)(std::streamsize), (), HttpClientException)

} // namespace
}
