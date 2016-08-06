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
#include <limits>
#include <memory>
#include <thread>
#include <cstring>

#include "curl/curl.h"

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/pimpl/pimpl_forward_macros.hpp"
#include "staticlib/httpclient/HttpResourceInfo.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace sc = staticlib::config;
namespace io = staticlib::io;

using headers_type = const std::vector<std::pair<std::string, std::string>>&;

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

class CurlSListDeleter {
public:
    void operator()(struct curl_slist* list) {
        curl_slist_free_all(list);
    }
};

class AppliedHeaders {
    std::vector<std::string> headers;
    std::unique_ptr<struct curl_slist, CurlSListDeleter> slist;
    
public:
    AppliedHeaders() { }
    
    AppliedHeaders(const AppliedHeaders&) = delete;
    
    AppliedHeaders& operator=(const AppliedHeaders&) = delete;

    void add_header(std::string header) {
        headers.emplace_back(std::move(header));
    }
    
    const std::string& get_last_header() {
        return headers.back();
    }

    void set_slist(struct curl_slist* slist_ptr) {
        slist.release();
        slist = std::unique_ptr<struct curl_slist, CurlSListDeleter>{slist_ptr, CurlSListDeleter{}};
    }
};

} // namespace


class HttpResource::Impl : public staticlib::pimpl::PimplObject::Impl {
    // holds data passed to curl
    HttpRequestOptions options;
    AppliedHeaders applied_headers;
    
    CURLM* multi_handle;
    std::unique_ptr<CURL, CurlEasyDeleter> handle;
    std::string url;
    std::unique_ptr<std::istream> post_data;
    
    HttpResourceInfo info;
    std::vector<char> buf;
    size_t buf_idx = 0;
    bool open = false;
    
public:
    Impl(CURLM* multi_handle,
            std::string url,
            std::unique_ptr<std::istream> post_data,
            HttpRequestOptions options) :
    options(std::move(options)),
    multi_handle(multi_handle),
    handle(curl_easy_init(), CurlEasyDeleter{multi_handle}),
    url(std::move(url)),
    post_data(std::move(post_data)) {
        if (nullptr == handle.get()) throw HttpClientException(TRACEMSG("Error initializing cURL handle"));
        CURLMcode errm = curl_multi_add_handle(multi_handle, handle.get());
        if (errm != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() +
                "cURL multi_add error: [" + sc::to_string(errm) + "], url: [" + this->url + "]"));
        apply_options();
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

    const HttpResourceInfo& get_info(const HttpResource&) const {
        return info;
    }
    
private:
    static size_t headers_callback(char* buffer, size_t size, size_t nitems, void* userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t>(-1);
        HttpResource::Impl* ptr = static_cast<HttpResource::Impl*> (userp);
        return ptr->write_headers(buffer, size, nitems);
    }

    static size_t write_callback(char* buffer, size_t size, size_t nitems, void* userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t> (-1);
        HttpResource::Impl* ptr = static_cast<HttpResource::Impl*> (userp);
        return ptr->write_data(buffer, size, nitems);
    }

    static size_t read_callback(char *buffer, size_t size, size_t nitems, void *userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t> (-1);
        HttpResource::Impl* ptr = static_cast<HttpResource::Impl*> (userp);
        return ptr->read_data(buffer, size, nitems);
    }

    // http://stackoverflow.com/a/9681122/314015
    size_t write_headers(char *buffer, size_t size, size_t nitems) {
        if (-1 == info.response_code) {
            info.effective_url = getinfo_string(CURLINFO_EFFECTIVE_URL);        
            info.response_code = getinfo_long(CURLINFO_RESPONSE_CODE);
        }
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

    size_t write_data(char *buffer, size_t size, size_t nitems) {
        size_t len = size*nitems;
        buf.resize(len);
        std::memcpy(buf.data(), buffer, len);
        return len;
    } 
    
    size_t read_data(char* buffer, size_t size, size_t nitems) {        
        std::streamsize len = static_cast<std::streamsize>(size * nitems);
        io::streambuf_source src{post_data->rdbuf()};
        std::streamsize read = io::read_all(src, buffer, len);
        return static_cast<size_t>(read);
    }
    
    // curl_multi_socket_action may be used instead of fdset
    void fill_buffer() {
        // some data in buffer
        if (buf_idx < buf.size()) return;
        buf_idx = 0;
        buf.resize(0);
        auto start = std::chrono::system_clock::now();
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
                std::this_thread::sleep_for(std::chrono::milliseconds{options.fdset_timeout_millis});
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

            // check for response code
            if (options.abort_on_response_error && info.response_code >= 400) {
                throw HttpClientException(TRACEMSG(std::string() +
                    "HTTP error returned from server, url: [" + url + "]," +
                    " response_code: [" + sc::to_string(info.response_code) + "]"));
            }
          
            // check for timeout
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
            if (elapsed.count() > options.read_timeout_millis) {
                throw HttpClientException(TRACEMSG(std::string() +
                    "Request timeout for url: [" + url + "]," +
                    " elapsed time (millis): [" + sc::to_string(elapsed.count()) + "]," +
                    " limit: [" + sc::to_string(options.read_timeout_millis) + "]"));
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
    
    double getinfo_double(CURLINFO opt) {
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
        // should be alreade set from headers callback, but won't harm here
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

    // note: think about integer overflow 
    void setopt_uint32(CURLoption opt, uint32_t value) {
        if (0 == value) return;
        if (value > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) throw HttpClientException(TRACEMSG(std::string() +
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to invalid overflow value: [" + sc::to_string(value) + "], url: [" + url + "]"));
        CURLcode err = curl_easy_setopt(handle.get(), opt, static_cast<long>(value));
        if (err != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to value: [" + sc::to_string(value) + "], url: [" + url + "]"));
    }

    void setopt_bool(CURLoption opt, bool value) {
        CURLcode err = curl_easy_setopt(handle.get(), opt, value ? 1 : 0);
        if (err != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to value: [" + sc::to_string(value) + "], url: [" + url + "]"));
    }

    void setopt_string(CURLoption opt, const std::string& value) {
        if ("" == value) return;
        CURLcode err = curl_easy_setopt(handle.get(), opt, value.c_str());
        if (err != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to value: [" + value + "], url: [" + url + "]"));
    }

    void setopt_object(CURLoption opt, void* value) {
        if (nullptr == value) return;
        CURLcode err = curl_easy_setopt(handle.get(), opt, value);
        if (err != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                "Error setting option: [" + sc::to_string(opt) + "], url: [" + url + "]"));
    }
    
    struct curl_slist* append_header(struct curl_slist* slist, const std::string& header) {
        struct curl_slist* res = curl_slist_append(slist, header.c_str());
        if (nullptr == res) throw HttpClientException(TRACEMSG(std::string() +
                "Error appending header: [" + header + "], url: [" + url + "]"));
        return res;
    }
    
    void apply_headers() {
        if (options.headers.empty()) return;
        struct curl_slist* slist = nullptr;
        for (auto& en : options.headers) {
            applied_headers.add_header(en.first + ": " + en.second);
            slist = append_header(slist, applied_headers.get_last_header());
            applied_headers.set_slist(slist);
        }
        setopt_object(CURLOPT_HTTPHEADER, static_cast<void*>(slist));
    }
    
    void appply_method() {
        if ("" == options.method) return;
        if ("GET" == options.method) {
            setopt_bool(CURLOPT_HTTPGET, true);
        } else if("POST" == options.method) {
            setopt_bool(CURLOPT_POST, true);
        } else if ("PUT" == options.method) {
            setopt_bool(CURLOPT_PUT, true);
        } else if ("DELETE" == options.method) {
            setopt_string(CURLOPT_CUSTOMREQUEST, "DELETE");
        } else throw HttpClientException(TRACEMSG(std::string() +
                    "Unsupported HTTP method: [" + options.method + "], url: [" + url + "]"));
        if (nullptr != post_data.get() && ("POST" == options.method || "PUT" == options.method)) {
            setopt_object(CURLOPT_READDATA, this);
            CURLcode err_wf = curl_easy_setopt(handle.get(), CURLOPT_READFUNCTION, HttpResource::Impl::read_callback);
            if (err_wf != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                    "Error setting option: [CURLOPT_READFUNCTION], url: [" + url + "]"));
            options.headers.emplace_back("Transfer-Encoding", "chunked");
        }
    }
    
    // todo: curl version checking
    void apply_options() {
        // url
        setopt_string(CURLOPT_URL, url);
        
        // method
        appply_method();
        
        // headers
        apply_headers();
        
        // callbacks
        setopt_object(CURLOPT_WRITEDATA, this);
        CURLcode err_wf = curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, HttpResource::Impl::write_callback);
        if (err_wf != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                "Error setting option: [CURLOPT_WRITEFUNCTION], url: [" + url + "]"));
        setopt_object(CURLOPT_HEADERDATA, this);
        CURLcode err_hf = curl_easy_setopt(handle.get(), CURLOPT_HEADERFUNCTION, HttpResource::Impl::headers_callback);
        if (err_hf != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                "Error setting option: [CURLOPT_HEADERFUNCTION], url: [" + url + "]"));
       
        // general behavior options
        if (options.force_http_10) {
            CURLcode err = curl_easy_setopt(handle.get(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
            if (err != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                    "Error setting option: [CURLOPT_HTTP_VERSION], url: [" + url + "]"));
        }
        setopt_bool(CURLOPT_NOPROGRESS, options.noprogress);
        setopt_bool(CURLOPT_NOSIGNAL, options.nosignal);
        setopt_bool(CURLOPT_FAILONERROR, options.failonerror);
//        setopt_bool(CURLOPT_PATH_AS_IS, options.path_as_is);

        // TCP options
        setopt_bool(CURLOPT_TCP_NODELAY, options.tcp_nodelay);
        setopt_bool(CURLOPT_TCP_KEEPALIVE, options.tcp_keepalive);
        setopt_uint32(CURLOPT_TCP_KEEPIDLE, options.tcp_keepidle_secs);
        setopt_uint32(CURLOPT_TCP_KEEPINTVL, options.tcp_keepintvl_secs);
        setopt_uint32(CURLOPT_CONNECTTIMEOUT_MS, options.connecttimeout_millis);

        // HTTP options
        setopt_uint32(CURLOPT_BUFFERSIZE, options.buffersize_bytes);
        setopt_string(CURLOPT_ACCEPT_ENCODING, options.accept_encoding);
        setopt_bool(CURLOPT_FOLLOWLOCATION, options.followlocation);
        setopt_uint32(CURLOPT_MAXREDIRS, options.maxredirs);
        setopt_string(CURLOPT_USERAGENT, options.useragent);

        // throttling options
        setopt_uint32(CURLOPT_MAX_SEND_SPEED_LARGE, options.max_sent_speed_large_bytes_per_second);
        setopt_uint32(CURLOPT_MAX_RECV_SPEED_LARGE, options.max_recv_speed_large_bytes_per_second);

        // SSL options
        setopt_string(CURLOPT_SSLCERT, options.sslcert_filename);
        setopt_string(CURLOPT_SSLCERTTYPE, options.sslcertype);
        setopt_string(CURLOPT_SSLKEY, options.sslkey_filename);
        setopt_string(CURLOPT_SSLKEYTYPE, options.ssl_key_type);
        setopt_string(CURLOPT_KEYPASSWD, options.ssl_keypasswd);
        if (options.require_tls) {
            CURLcode err = curl_easy_setopt(handle.get(), CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
            if (err != CURLE_OK) throw HttpClientException(TRACEMSG(std::string() +
                "Error setting option: [CURLOPT_SSLVERSION], url: [" + url + "]"));
        }
        setopt_uint32(CURLOPT_SSL_VERIFYHOST, options.ssl_verifyhost ? 2 : 0);
        setopt_bool(CURLOPT_SSL_VERIFYPEER, options.ssl_verifypeer);
//        setopt_bool(CURLOPT_SSL_VERIFYSTATUS, options.ssl_verifystatus);
        setopt_string(CURLOPT_CAINFO, options.cainfo_filename);
        setopt_string(CURLOPT_CRLFILE, options.crlfile_filename);
        setopt_string(CURLOPT_SSL_CIPHER_LIST, options.ssl_cipher_list);
    }

    
};

PIMPL_FORWARD_CONSTRUCTOR(HttpResource, (CURLM*)(std::string)(std::unique_ptr<std::istream>)(HttpRequestOptions), (), HttpClientException)
PIMPL_FORWARD_METHOD(HttpResource, std::streamsize, read, (char*)(std::streamsize), (), HttpClientException)
PIMPL_FORWARD_METHOD(HttpResource, const HttpResourceInfo&, get_info, (), (const), HttpClientException)

} // namespace
}
