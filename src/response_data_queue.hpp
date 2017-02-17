/* 
 * File:   response_data_queue.hpp
 * Author: alex
 *
 * Created on February 13, 2017, 8:07 PM
 */

#ifndef STATICLIB_HTTPCLIENT_RESPONSE_DATA_QUEUE_HPP
#define	STATICLIB_HTTPCLIENT_RESPONSE_DATA_QUEUE_HPP

#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "staticlib/containers.hpp"

namespace staticlib {
namespace httpclient {

class response_data_queue : public std::enable_shared_from_this<response_data_queue> {
    std::mutex mutex;
    std::condition_variable cv;
    staticlib::containers::producer_consumer_queue<std::vector<char>> queue;
    // separate flag used instead of poison pill
    // due to limited queue size
    bool exhausted = false;

public:
    response_data_queue(size_t queue_size) :
    queue(queue_size) { }

    // producer methods
           
    bool emplace(std::vector<char>&& buffer) {
        bool placed = queue.emplace(std::move(buffer));
        if (placed) {
            cv.notify_all();
        }
        return placed;
    }

    void finalize() STATICLIB_NOEXCEPT {
        {
            std::lock_guard<std::mutex> guard{mutex};
            exhausted = true;
        }
        cv.notify_all();
    }
    
    bool is_full() {
        return queue.is_full();
    }
    
    bool is_empty() {
        return queue.is_empty();
    }

    // consumer methods

    bool poll(std::vector<char>& dest_buffer) {
        return queue.poll(dest_buffer);
    }
    
    bool take(std::vector<char>& dest_buffer) {
        std::unique_lock<std::mutex> guard{mutex};
        cv.wait(guard, [this] {
            return !(!exhausted && queue.is_empty());
        });
        return queue.poll(dest_buffer);
    }
    
    size_t size() {
        return queue.size_guess();
    }
};


} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_RESPONSE_DATA_QUEUE_HPP */

