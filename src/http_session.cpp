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

// TODO: removeme
#include <iostream>
// TODO: end removeme

#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "curl/curl.h"

#include "staticlib/containers.hpp"
#include "staticlib/io.hpp"
#include "staticlib/pimpl/pimpl_forward_macros.hpp"

#include "curl_deleters.hpp"
#include "curl_headers.hpp"
#include "response_data_queue.hpp"
#include "running_request_pipe.hpp"
#include "running_request.hpp"
#include "request_tickets_queue.hpp"
#include "http_resource_params.hpp"
#include "all_requests_paused_latch.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

namespace sc = staticlib::config;
namespace io = staticlib::io;

//

} // namespace

class http_session::impl : public staticlib::pimpl::pimpl_object::impl {
    http_session_options options;
    std::unique_ptr<CURLM, curl_multi_deleter> handle;
    
    request_tickets_queue tickets;
    std::map<int64_t, std::unique_ptr<running_request>> requests;

    std::thread worker;
    std::atomic<bool> running;
    
    std::shared_ptr<all_requests_paused_latch> pause_latch = std::make_shared<all_requests_paused_latch>();

public:
    impl(http_session_options options) :
    options(options),
    handle(curl_multi_init(), curl_multi_deleter()),
    tickets(options.requests_queue_max_size),
    running(true) { 
        if (nullptr == handle.get()) throw httpclient_exception(TRACEMSG("Error initializing cURL multi handle"));
        // available since 7.30.0        
#if LIBCURL_VERSION_NUM >= 0x071e00
        setopt_uint32(CURLMOPT_MAX_HOST_CONNECTIONS, options.max_host_connections);
        setopt_uint32(CURLMOPT_MAX_TOTAL_CONNECTIONS, options.max_total_connections);
#endif // LIBCURL_VERSION_NUM
        setopt_uint32(CURLMOPT_MAXCONNECTS, options.maxconnects);
        worker = std::thread([this] {
            while (running.load()) {
                // wait on queue
                request_ticket ticket;
                auto got_ticket = tickets.take(ticket);
                if (!got_ticket) { // destruction in progress
                    break; 
                }
                enqueue_request(std::move(ticket));
                
                uint64_t spinwait_cycles = 0;
                uint64_t spinwait_enter_cycles = 0;
                uint64_t spinwait_exit_cycles = 0;
                
                // spin while has active transfers
                while(requests.size() > 0) {
                    // timeout
                    long timeo = -1;
                    CURLMcode err_timeout = curl_multi_timeout(handle.get(), std::addressof(timeo));
                    if (check_and_abort_on_multi_error(err_timeout)) break;
                    struct timeval timeout = create_timeout_struct(timeo);
                    
                    // fdset
                    fd_set fdread = create_fd();
                    fd_set fdwrite = create_fd();
                    fd_set fdexcep = create_fd();
                    int maxfd = -1;
                    CURLMcode err_fdset = curl_multi_fdset(handle.get(), std::addressof(fdread),
                            std::addressof(fdwrite), std::addressof(fdexcep), std::addressof(maxfd));
                    if (check_and_abort_on_multi_error(err_fdset)) break;

                    // wait or select
                    int err_select = 0;
                    if (maxfd == -1) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(this->options.fdset_timeout_millis));
                    } else {
                        err_select = select(maxfd + 1, std::addressof(fdread), std::addressof(fdwrite),
                                std::addressof(fdexcep), std::addressof(timeout));
                    }

                    // do perform if no select error
                    int active = -1;
                    if (-1 != err_select) {
                        CURLMcode err_perform = curl_multi_perform(handle.get(), std::addressof(active));
                        if (check_and_abort_on_multi_error(err_perform)) break;
                    }
                    
                    // pop completed or timeouted
                    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                    for(;;) {
                        int tmp = -1;
                        CURLMsg* m = curl_multi_info_read(handle.get(), std::addressof(tmp));
                        if (nullptr == m) break;
                        int64_t req_key = reinterpret_cast<int64_t> (m->easy_handle);
                        auto it = requests.find(req_key);
                        if (requests.end() == it) {
                            abort_running_on_multi_error(TRACEMSG("System error: inconsistent queue state, aborting"));
                            break;
                        }
                        running_request& req = *it->second;
                        if (CURLMSG_DONE == m->msg) {
                            CURLcode result = m->data.result;
                            if (req.get_options().abort_on_connect_error && CURLE_OK != result) {
                                req.append_error(curl_easy_strerror(result));
                            }
                            std::cout << "is due to finalize: " << req.get_url() << 
                                    ", spinwait_cycles: " << spinwait_cycles << 
                                    ", spinwait_exit_cycles: " << spinwait_exit_cycles <<
                                    ", spinwait_enter_cycles: " << spinwait_enter_cycles << std::endl;
                            requests.erase(req_key);
                        } else {
                            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - req.started_at());
                            if (elapsed.count() > req.get_options().read_timeout_millis) {
                                req.append_error(TRACEMSG("Request timeout error," +
                                        " elapsed time (millis): [" + sc::to_string(elapsed.count()) + "]," +
                                        " limit: [" + sc::to_string(req.get_options().read_timeout_millis) + "]"));
                                requests.erase(req_key);
                            }
                        }
                    }
                    
                    if (0 == requests.size()) break;
                    
                    // spin wait for paused
                    for(;;) {
                        spinwait_enter_cycles += 1;
                        // unpause when possible
                        size_t num_paused = 0;
                        for (auto& pa : requests) {
                            running_request& req = *pa.second;
                            if (req.is_paused()) {
                                if (!req.data_queue_is_full()) {
                                    req.unpause();
                                } else {
                                    num_paused += 1;
                                }
                            }
                        }

                        // check more tickets
                        auto newcomers = poll_enqueued_tickets();
                        for (request_ticket& ti : newcomers) {
                            enqueue_request(std::move(ti));
                        }
                                                
                        // whether to end spinwait
                        if (newcomers.size() > 0 || requests.size() > num_paused) {
                            spinwait_exit_cycles += 1;
                            break;
                        }
                        
                        spinwait_cycles += 1;
                        pause_latch->await();
                    }
                }
            }
        });
    }
    
    /**
     * managed all handles inside this class
     * 
     */
    
    ~impl() STATICLIB_NOEXCEPT {
        running.store(false);
        tickets.unblock();
        worker.join();
    }

    http_resource open_url(http_session& frontend,
            const std::string& url,
            http_request_options options) {
        if ("" == options.method) {
            options.method = "GET";
        }
        return open_url(frontend, url, nullptr, std::move(options));
    }
    
    http_resource open_url(http_session& frontend,
            const std::string& url,
            std::streambuf* post_data, http_request_options options) {
        namespace si = staticlib::io;
        auto sbuf_ptr = [post_data] () -> std::streambuf* {
            if (nullptr != post_data) {
                return post_data;
            }
            static std::istringstream empty_stream{""};
            return empty_stream.rdbuf();
        } ();
        if ("" == options.method) {
            options.method = "POST";
        }
        auto sbuf = si::make_source_istream_ptr(si::streambuf_source(sbuf_ptr));
        return open_url(frontend, url, std::move(sbuf), std::move(options));
    }
    
    http_resource open_url(http_session&, const std::string& url,
            std::unique_ptr<std::istream> post_data, http_request_options options) {
        if ("" == options.method) {
            options.method = "POST";
        }
        //  note: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63736
        auto pipe = std::make_shared<running_request_pipe>(pause_latch);
        auto enqueued = tickets.emplace(url, std::move(options), std::move(post_data), pipe);
        if (!enqueued) throw httpclient_exception(TRACEMSG(
                "Requests queue is full, size: [" + sc::to_string(tickets.size()) + "]"));
        auto params = http_resource_params(url, std::move(pipe));
        return http_resource(std::move(params));
    }
    
private:
    void setopt_uint32(CURLMoption opt, uint32_t value) {
        if (0 == value) return;
        CURLMcode err = curl_multi_setopt(handle.get(), opt, value);
        if (err != CURLM_OK) throw httpclient_exception(TRACEMSG(
                "Error setting session option: [" + sc::to_string(opt) + "]," +
                " to value: [" + sc::to_string(value) + "]"));
    }
    
    void enqueue_request(request_ticket&& ticket) {
        // local copy                
        auto pipe = ticket.pipe;
        try {
            auto req = std::unique_ptr<running_request>(new running_request(handle.get(), std::move(ticket)));
            auto ha = req->easy_handle();
            auto pa = std::make_pair(reinterpret_cast<int64_t> (ha), std::move(req));
            requests.insert(std::move(pa));
        } catch (const std::exception& e) {
            // this two are normally called
            // on requests queue pop, but here
            // enqueue itself is failed
            pipe->append_error(TRACEMSG(e.what()));
            pipe->finalize_data_queue();
        }
    }
    
    std::vector<request_ticket> poll_enqueued_tickets() {
        std::vector<request_ticket> vec;
        tickets.consume([&vec](request_ticket&& ti){
            vec.emplace_back(std::move(ti));
        });
        return vec;
    }
    
    bool check_and_abort_on_multi_error(CURLMcode code) {
        if (CURLM_OK == code) return false;
        abort_running_on_multi_error(TRACEMSG("CURL engine error, transfer aborted," +
                " code: [" + curl_multi_strerror(code) + "]"));
        return true;
    }
    
    void abort_running_on_multi_error(const std::string& error) {
        for (auto& pa : requests) {
            running_request& re = *pa.second;
            re.append_error(error);
        }
        requests.clear();
    }

    // http://curl-library.cool.haxx.narkive.com/2sYifbgu/issue-with-curl-multi-timeout-while-doing-non-blocking-http-posts-in-vms
    struct timeval create_timeout_struct(long timeo) {
        struct timeval timeout;
        std::memset(std::addressof(timeout), '\0', sizeof (timeout));
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

    
};
PIMPL_FORWARD_CONSTRUCTOR(http_session, (http_session_options), (), httpclient_exception)
PIMPL_FORWARD_METHOD(http_session, http_resource, open_url, (const std::string&)(http_request_options), (), httpclient_exception)
PIMPL_FORWARD_METHOD(http_session, http_resource, open_url, (const std::string&)(std::streambuf*)(http_request_options), (), httpclient_exception)
PIMPL_FORWARD_METHOD(http_session, http_resource, open_url, (const std::string&)(std::unique_ptr<std::istream>)(http_request_options), (), httpclient_exception)        

} // namespace
}
