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
 * File:   multi_threaded_session.cpp
 * Author: alex
 *
 * Created on March 25, 2017, 6:20 PM
 */

#include "staticlib/http/multi_threaded_session.hpp"

#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "curl/curl.h"

#include "staticlib/concurrent.hpp"
#include "staticlib/io.hpp"
#include "staticlib/pimpl/forward_macros.hpp"

#include "session_impl.hpp"
#include "curl_headers.hpp"
#include "curl_utils.hpp"
#include "resource_params.hpp"
#include "multi_threaded_resource.hpp"
#include "running_request_pipe.hpp"
#include "running_request.hpp"

namespace staticlib {
namespace http {

class multi_threaded_session::impl : public session::impl {
    sl::concurrent::mpmc_blocking_queue<request_ticket> tickets;
    std::atomic<bool> new_tickets_arrived;
    std::map<int64_t, std::unique_ptr<running_request>> requests;

    std::shared_ptr<sl::concurrent::condition_latch> pause_latch;

    std::thread worker;
    std::atomic<bool> running;
    
public:
    impl(session_options options) :
    session::impl(options),    
    tickets(options.requests_queue_max_size),
    new_tickets_arrived(false),
    pause_latch(std::make_shared<sl::concurrent::condition_latch>([this] {
        return this->check_pause_condition();
    })),
    running(true) {
        worker = std::thread([this] {
            this->worker_proc();
        });
    }

    ~impl() STATICLIB_NOEXCEPT {
        running.store(false, std::memory_order_release);
        pause_latch->notify_one();
        tickets.unblock();
        worker.join();
    }

    resource open_url(multi_threaded_session&, const std::string& url,
            std::unique_ptr<std::istream> post_data, request_options options) {
        if ("" == options.method) {
            options.method = "POST";
        }
        //  note: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63736
        auto pipe = std::make_shared<running_request_pipe>(options, pause_latch);
        auto enqueued = tickets.emplace(url, std::move(options), std::move(post_data), pipe);
        if (!enqueued) throw http_exception(TRACEMSG(
                "Requests queue is full, size: [" + sl::support::to_string(tickets.size()) + "]"));
        new_tickets_arrived.exchange(true, std::memory_order_acq_rel);
        pause_latch->notify_one();
        auto params = resource_params(url, std::move(pipe));
        return multi_threaded_resource(std::move(params));
    }

    // not exposed

    bool check_pause_condition() {
        // unpause when possible
        size_t num_paused = unpause_enqueued_requests();

        // check more tickets
        if (new_tickets_arrived.load(std::memory_order_acquire)) {
            new_tickets_arrived.store(false, std::memory_order_release);
            tickets.poll([this](request_ticket && ti) {
                this->enqueue_request(std::move(ti));
            });
        }
        
        if (requests.size() > num_paused) {
            // consumers are active, let's proceed
            return true;
        }

        // check we are still running
        return !running.load(std::memory_order_acquire);
    }

private:

    void worker_proc() {
        while (running.load(std::memory_order_acquire)) {
            // wait on queue
            request_ticket ticket;
            auto got_ticket = tickets.take(ticket);
            if (!got_ticket) { // destruction in progress
                break;
            }
            enqueue_request(std::move(ticket));

            // spin while has active transfers
            while (requests.size() > 0) {
                
                // receive data
                bool perform_success = curl_perform();
                if (!perform_success) {
                    break;
                }

                // pop completed
                bool pop_success = pop_completed_requests();
                if (!pop_success) {
                    break;
                }
                
                // check there are running requests
                if (0 == requests.size()) break;

                // wait if all requests paused
                pause_latch->await();
            }
        }
        requests.clear();
    }

    bool curl_perform() {
        // timeout
        long timeo = -1;
        CURLMcode err_timeout = curl_multi_timeout(handle.get(), std::addressof(timeo));
        if (check_and_abort_on_multi_error(err_timeout)) {
            return false;
        }
        struct timeval timeout = create_timeout_struct(timeo, this->options.mt_socket_select_max_timeout_millis);
        
        // fdset
        fd_set fdread = create_fd();
        fd_set fdwrite = create_fd();
        fd_set fdexcep = create_fd();
        int maxfd = -1;
        CURLMcode err_fdset = curl_multi_fdset(handle.get(), std::addressof(fdread),
                std::addressof(fdwrite), std::addressof(fdexcep), std::addressof(maxfd));
        if (check_and_abort_on_multi_error(err_fdset)) {
            return false;
        }

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
            if (check_and_abort_on_multi_error(err_perform)) {
                return false;
            }
        }

        return true;
    }

    bool pop_completed_requests() {
        for (;;) {
            int tmp = -1;
            CURLMsg* cm = curl_multi_info_read(handle.get(), std::addressof(tmp));
            if (nullptr == cm) {
                // all requests in queue inspected
                break;
            }
            int64_t req_key = reinterpret_cast<int64_t> (cm->easy_handle);
            auto it = requests.find(req_key);
            if (requests.end() == it) {
                abort_running_on_multi_error(TRACEMSG("System error: inconsistent queue state, aborting"));
                return false;
            }
            running_request& req = *it->second;
            if (CURLMSG_DONE == cm->msg) {
                CURLcode result = cm->data.result;
                if (req.get_options().abort_on_connect_error && CURLE_OK != result) {
                    req.append_error(curl_easy_strerror(result));
                }
                requests.erase(req_key);
            }
        }
        return true;
    }

    size_t unpause_enqueued_requests() {
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
        return num_paused;
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
            // these two lines are normally called
            // on requests queue pop, but here
            // enqueue itself failed
            pipe->append_error(TRACEMSG(e.what()));
            pipe->shutdown();
        }
    }

    bool check_and_abort_on_multi_error(CURLMcode code) {
        if (CURLM_OK == code) return false;
        abort_running_on_multi_error(TRACEMSG("cURL engine error, transfer aborted," +
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
};
PIMPL_FORWARD_CONSTRUCTOR(multi_threaded_session, (session_options), (), http_exception)
PIMPL_FORWARD_METHOD(multi_threaded_session, resource, open_url, (const std::string&)(std::unique_ptr<std::istream>)(request_options), (), http_exception)            

} // namespace
}
