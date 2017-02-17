/* 
 * File:   running_request_pipe.hpp
 * Author: alex
 *
 * Created on February 14, 2017, 5:27 PM
 */

#ifndef STATICLIB_HTTPCLIENT_RUNNING_REQUEST_PIPE_HPP
#define	STATICLIB_HTTPCLIENT_RUNNING_REQUEST_PIPE_HPP

#include <cstdint>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

#include "staticlib/config.hpp"

#include "staticlib/httpclient/httpclient_exception.hpp"
#include "staticlib/httpclient/http_resource_info.hpp"

#include "response_data_queue.hpp"
#include "all_requests_paused_latch.hpp"

namespace staticlib {
namespace httpclient {

class running_request_pipe : public std::enable_shared_from_this<running_request_pipe> {
    std::atomic<int16_t> response_code;
    staticlib::containers::producer_consumer_queue<http_resource_info> resource_info;
    response_data_queue data_queue;
    staticlib::containers::producer_consumer_queue<std::pair<std::string, std::string>> headers_queue;
    std::atomic<bool> errors_non_empty;
    staticlib::containers::synchronized_queue<std::string> errors;
    
    std::shared_ptr<all_requests_paused_latch> pause_latch;

public:    
    running_request_pipe(std::shared_ptr<all_requests_paused_latch> pause_latch) :
    response_code(0),
    resource_info(1),
    data_queue(16),
    headers_queue(128),
    errors_non_empty(false),
    errors(std::numeric_limits<uint16_t>::max()),
    pause_latch(std::move(pause_latch)) { }
    
    running_request_pipe(const running_request_pipe&) = delete;
    
    running_request_pipe& operator=(const running_request_pipe&) = delete;
    
    void set_response_code(long code) {
        namespace sc = staticlib::config;
        if (!sc::is_uint16_positive(code)) throw httpclient_exception(TRACEMSG(
                "Invalid response code specified: [" + sc::to_string(code) + "]"));
        int16_t the_zero = 0;
        bool success = response_code.compare_exchange_strong(the_zero, static_cast<int16_t> (code));
        if (!success) throw httpclient_exception(TRACEMSG(
                "Invalid second attempt to set response code," +
                " existing code: [" + sc::to_string(response_code.load()) +"]," +
                " new code: [" + sc::to_string(code) + "]"));
    }
    
    uint16_t get_response_code() const {
        return response_code.load();
    }
    
    void set_resource_info(http_resource_info&& info) {
        if (resource_info.size_guess() > 0) throw httpclient_exception(TRACEMSG(
                "Invalid second attempt to set resource info"));
        resource_info.emplace(std::move(info));
    }
    
    http_resource_info get_resource_info() {
        http_resource_info res;
        resource_info.poll(res);
        return res;
    }
    
    bool emplace_some_data(std::vector<char>&& data) {
        return data_queue.emplace(std::move(data));
    }
    
    bool receive_some_data(std::vector<char>& dest_buffer) {
        bool prefilled = data_queue.poll(dest_buffer);
        bool res;
        if (prefilled) {
            res = true;
        } else {
            res = data_queue.take(dest_buffer);
        }
        if (res && data_queue.is_empty()) {
            pause_latch->unlock();
        }
        return res;
    }
    
    void finalize_data_queue() STATICLIB_NOEXCEPT {
        data_queue.finalize();
    }
    
    bool data_queue_is_full() {
        return data_queue.is_full();
    }
    
    void emplace_header(std::string&& name, std::string&& value) {
        namespace sc = staticlib::config;
        bool emplaced = headers_queue.emplace(std::move(name), std::move(value));
        if (!emplaced) throw httpclient_exception(TRACEMSG(
                "Error emplacing header to queue, " +
                "queue size: [" + sc::to_string(headers_queue.max_size()) + "]"));        
    }
    
    std::vector<std::pair<std::string, std::string>> consume_received_headers() {
        std::vector<std::pair<std::string, std::string>> res;
        auto pa = std::pair<std::string, std::string>();
        while (headers_queue.poll(pa)) {
            res.emplace_back(std::move(pa));
        }
        return res;
    }
    
    // not synchronized, but only single producer is expected
    void append_error(const std::string& msg) STATICLIB_NOEXCEPT {
        errors_non_empty.store(true);
        errors.emplace(msg);
    }
    
    std::string get_error_message() {
        std::string dest;
        errors.consume([&dest](std::string&& st){
            if (!dest.empty()) {
                dest.append("\n");
            }
            dest.append(st);
        });
        return dest;
    }
    
    bool has_errors() {
        return errors_non_empty.load();
    }
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_RUNNING_REQUEST_PIPE_HPP */

