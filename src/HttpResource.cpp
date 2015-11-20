/* 
 * File:   HttpResource.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:43 AM
 */

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>

#include "curl/curl.h"

#include "staticlib/utils.hpp"
#include "staticlib/pimpl/pimpl_forward_macros.hpp"

#include "staticlib/httpclient/HttpClientException.hpp"
#include "staticlib/httpclient/HttpResource.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace su = staticlib::utils;

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
    Impl(CURLM* multi_handle, std::string url_string) :
    multi_handle(multi_handle),
    handle(curl_easy_init(), CurlEasyDeleter{multi_handle}),
    url(std::move(url_string)) {
        if (nullptr == handle.get()) throw HttpClientException(TRACEMSG("Error initialising cURL handle"));
        CURLMcode err =  curl_multi_add_handle(multi_handle, handle.get());
        if (err != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() +
                "cURL multi_add error: [" + su::to_string(err) + "]"));
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
    static size_t write_callback(char* buffer, size_t size, size_t nitems, void* userp) {
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
    
    void fill_buffer() {
        // some data in buffer
        if (buf_idx < buf.size()) return;
        buf_idx = 0;
        buf.resize(0);
        /* attempt to fill buffer */
        do {
            int maxfd = -1;
            long curl_timeo = -1;

            /* set a suitable timeout to fail on */
            struct timeval timeout;
            std::memset(std::addressof(timeout), '\0', sizeof(timeout));
            timeout.tv_sec = 10; /* 1 minute */
            timeout.tv_usec = 0;

            // todo
            curl_multi_timeout(multi_handle, &curl_timeo);
            if (curl_timeo >= 0) {
                timeout.tv_sec = curl_timeo / 1000;
                if (timeout.tv_sec > 1)
                    timeout.tv_sec = 1;
                else
                    timeout.tv_usec = (curl_timeo % 1000) * 1000;
            }

            /* get file descriptors from the transfers */
            fd_set fdread;
            FD_ZERO(&fdread);
            fd_set fdwrite;
            FD_ZERO(&fdwrite);
            fd_set fdexcep;
            FD_ZERO(&fdexcep);
            CURLMcode mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

            if (mc != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() + 
                    "cURL fdset error: [" + su::to_string(mc) + "]"));

            /* On success the value of maxfd is guaranteed to be >= -1. We call
               select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
               no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
               to sleep 100ms, which is the minimum suggested value in the
               curl_multi_fdset() doc. */

            int rc = 0;
            if (maxfd == -1) {
                std::this_thread::sleep_for(std::chrono::microseconds{100});
            } else {
                /* Note that on some platforms 'timeout' may be modified by select().
                   If you need access to the original value save a copy beforehand. */
                rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
            }

            switch (rc) {
            case -1:
                /* select error */
                break;
            case 0:
            default:
                /* timeout or readable/writable sockets */
                int active = -1;
                CURLMcode err = curl_multi_perform(multi_handle, std::addressof(active));
                if (err != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() +
                        "cURL multi_perform error: [" + su::to_string(err) + "]"));
                open = (1 == active);
                break;
            }
        } while (open && 0 == buf.size());
    }    
    
};
PIMPL_FORWARD_CONSTRUCTOR(HttpResource, (CURLM*)(std::string), (), HttpClientException)
PIMPL_FORWARD_METHOD(HttpResource, std::streamsize, read, (char*)(std::streamsize), (), HttpClientException)

} // namespace
}
