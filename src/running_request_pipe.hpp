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
 * File:   running_request_pipe.hpp
 * Author: alex
 *
 * Created on February 14, 2017, 5:27 PM
 */

#ifndef STATICLIB_HTTP_RUNNING_REQUEST_PIPE_HPP
#define STATICLIB_HTTP_RUNNING_REQUEST_PIPE_HPP

#include <cstdint>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

#include "staticlib/config.hpp"
#include "staticlib/concurrent.hpp"

#include "staticlib/http/http_exception.hpp"
#include "staticlib/http/request_options.hpp"
#include "staticlib/http/resource_info.hpp"

namespace staticlib {
namespace http {

class running_request_pipe : public std::enable_shared_from_this<running_request_pipe> {
    std::atomic<int16_t> response_code;
    sl::concurrent::spsc_inobject_concurrent_queue<resource_info, 1> res_info;
    sl::concurrent::spsc_inobject_waiting_queue<sl::concurrent::growing_buffer, 16> data_queue;
    sl::concurrent::spsc_concurrent_queue<std::pair<std::string, std::string>> headers_queue;
    std::atomic<bool> errors_non_empty;
    sl::concurrent::mpmc_blocking_queue<std::string> errors;
    std::shared_ptr<sl::concurrent::condition_latch> pause_latch;
    uint16_t consumer_thread_wakeup_timeout_millis;
    std::atomic<bool> running;

public:
    running_request_pipe(request_options& opts, 
            std::shared_ptr<sl::concurrent::condition_latch> pause_latch) :
    response_code(0),
    headers_queue(opts.max_number_of_response_headers),
    errors_non_empty(false),
    errors(std::numeric_limits<uint16_t>::max()),
    pause_latch(std::move(pause_latch)),
    consumer_thread_wakeup_timeout_millis(opts.consumer_thread_wakeup_timeout_millis),
    running(true) { }

    running_request_pipe(const running_request_pipe&) = delete;

    running_request_pipe& operator=(const running_request_pipe&) = delete;

    void set_response_code(long code) {
        if (!sl::support::is_uint16_positive(code)) throw http_exception(TRACEMSG(
                "Invalid response code specified: [" + sl::support::to_string(code) + "]"));
        int16_t the_zero = 0;
        bool success = response_code.compare_exchange_strong(the_zero, static_cast<int16_t> (code),
                std::memory_order_release, std::memory_order_relaxed);
        if (!success) throw http_exception(TRACEMSG(
                "Invalid second attempt to set response code," +
                " existing code: [" + sl::support::to_string(the_zero) +"]," +
                " new code: [" + sl::support::to_string(code) + "]"));
    }

    uint16_t get_response_code() const {
        return response_code.load(std::memory_order_acquire);
    }

    void set_resource_info(resource_info&& info) {
        if (res_info.size() > 0) throw http_exception(TRACEMSG(
                "Invalid second attempt to set resource info"));
        res_info.emplace(std::move(info));
    }

    resource_info get_resource_info() {
        resource_info res;
        res_info.poll(res);
        return res;
    }

    bool write_some_data(sl::concurrent::growing_buffer&& buf) {
        return data_queue.emplace(std::move(buf));
    }

    bool receive_some_data(sl::concurrent::growing_buffer& dest_buffer) {
        // non-blocking read
        bool polres = data_queue.poll(dest_buffer);
        if (polres) {
            return true;
        }
        // blocking read
        pause_latch->notify_one();
        // at this point we have a race condition between waiting consumer 
        // and worker that is due to start waiting on pause_latch;
        // synchronization appeared to be too expensive here,
        // so this spinwait + timeout trick is used
        for(;;) {
            auto timeout = std::chrono::milliseconds(consumer_thread_wakeup_timeout_millis);
            bool takeres = data_queue.take(dest_buffer, timeout);
            if (takeres) {
                // got data
                return true;
            }
            if (running.load(std::memory_order_acquire)) {
                // still running, wakeup worker and wait again
                pause_latch->notify_one();
            } else {
                // shutting down
                return data_queue.take(dest_buffer);
            }
        }
    }

    void shutdown() STATICLIB_NOEXCEPT {
        running.store(false, std::memory_order_release);
        data_queue.unblock();
    }

    bool data_queue_is_full() {
        return data_queue.full();
    }

    void emplace_header(std::pair<std::string, std::string>&& pair) {
        bool emplaced = headers_queue.emplace(std::move(pair));
        if (!emplaced) throw http_exception(TRACEMSG(
                "Error emplacing header to queue, " +
                "queue size: [" + sl::support::to_string(headers_queue.max_size()) + "]"));
    }

    std::vector<std::pair<std::string, std::string>> consume_received_headers() {
        std::vector<std::pair<std::string, std::string>> res;
        auto pa = std::pair<std::string, std::string>();
        while (headers_queue.poll(pa)) {
            res.emplace_back(std::move(pa));
        }
        return res;
    }

    void append_error(const std::string& msg) STATICLIB_NOEXCEPT {
        errors_non_empty.store(true, std::memory_order_release);
        errors.emplace(msg);
    }

    std::string get_error_message() {
        std::string dest;
        errors.poll([&dest](std::string&& st){
            if (!dest.empty()) {
                dest.append("\n");
            }
            dest.append(st);
        });
        return dest;
    }
    
    bool has_errors() {
        return errors_non_empty.load(std::memory_order_acquire);
    }

};

} // namespace
}

#endif /* STATICLIB_HTTP_RUNNING_REQUEST_PIPE_HPP */

