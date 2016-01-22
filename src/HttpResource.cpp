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


namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace sc = staticlib::config;

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
    
    std::vector<char> buf;
    size_t buf_idx = 0;
    bool open = false;
    
public:
    Impl(CURLM* multi_handle, 
            const std::string& url_in,
            const std::string& method,
            const std::streambuf& data,
            const std::vector<std::pair<std::string, std::string>>& headers,
            const std::string& ssl_ca_file,
            const std::string& ssl_cert_file,
            const std::string& ssl_key_file,
            const std::string& ssl_key_passwd) :
    multi_handle(multi_handle),
    handle(curl_easy_init(), CurlEasyDeleter{multi_handle}),
    url(url_in) {
        if (nullptr == handle.get()) throw HttpClientException(TRACEMSG("Error initializing cURL handle"));
        CURLMcode errm = curl_multi_add_handle(multi_handle, handle.get());
        if (errm != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() +
                "cURL multi_add error: [" + sc::to_string(errm) + "], url: [" + url + "]"));
//        CURLcode err = CURLE_OK;
        // method options
////        switch (method) {
////        case "GET": break;
////        case "POST":            
////            err = curl_easy_setopt(handle.get(), CURLOPT_POST, 1);
////            if (err != CURLE_OK) throw HttpClientException(TRACEMSG("cURL CURLOPT_POST error: [" + sc::to_string(err) + "]"));    
////            // todo
////        case "PUT":
////        case "DELETE":
////        default: throw HttpClientException(TRACEMSG(std::string() + "Unsupported method: [" + method + "]"));    
//        }
        (void) method;
        (void) data;
        (void) headers;
        (void) ssl_ca_file;
        (void) ssl_cert_file;
        (void) ssl_key_file;
        (void) ssl_key_passwd;
        curl_easy_setopt(handle.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, this);
        curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, HttpResource::Impl::write_callback);
        open = true;
    }

    std::streamsize read(HttpResource&, char* buffer, std::streamsize length) {
        size_t ulen = static_cast<size_t> (length);
        fill_buffer();
        if (0 == buf.size()) {
            return open ? 0 : std::char_traits<char>::eof();
        }
        // return from buffer
        size_t avail = buf.size() - buf_idx;
        size_t reslen = avail <= ulen ? avail : ulen;
        std::memcpy(buffer, buf.data(), reslen);
        buf_idx += reslen;
        return static_cast<std::streamsize>(reslen);
    }
    
private:
    static size_t write_callback(char* buffer, size_t size, size_t nitems, void* userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return -1;
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
            // http://curl-library.cool.haxx.narkive.com/2sYifbgu/issue-with-curl-multi-timeout-while-doing-non-blocking-http-posts-in-vms
            long timeo = -1;
            CURLMcode err_timeout = curl_multi_timeout(multi_handle, &timeo);
            if (err_timeout != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() +
                    "cURL timeout error: [" + sc::to_string(err_timeout) + "], url: [" + url + "]"));
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

            // get file descriptors from the transfers
            fd_set fdread;
            FD_ZERO(std::addressof(fdread));
            fd_set fdwrite;
            FD_ZERO(std::addressof(fdwrite));
            fd_set fdexcep;
            FD_ZERO(std::addressof(fdexcep));
            int maxfd = -1;
            CURLMcode err_fdset = curl_multi_fdset(multi_handle, std::addressof(fdread), 
                    std::addressof(fdwrite), std::addressof(fdexcep), std::addressof(maxfd));
            if (err_fdset != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() + 
                    "cURL fdset error: [" + sc::to_string(err_fdset) + "], url: [" + url + "]"));

            // wait or select
            int err_select = 0;
            if (maxfd == -1) {
                std::this_thread::sleep_for(std::chrono::microseconds{100});
            } else {
                err_select = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
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
    
};
PIMPL_FORWARD_CONSTRUCTOR(HttpResource, 
        (CURLM*)
        (const std::string&)
        (const std::string&)
        (const std::streambuf&)
        (headers_type)
        (const std::string&)
        (const std::string&)
        (const std::string&)
        (const std::string&),
        (), HttpClientException)
PIMPL_FORWARD_METHOD(HttpResource, std::streamsize, read, (char*)(std::streamsize), (), HttpClientException)

} // namespace
}
