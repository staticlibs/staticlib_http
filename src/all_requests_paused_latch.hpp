/* 
 * File:   all_requests_paused_latch.hpp
 * Author: alex
 *
 * Created on February 17, 2017, 11:14 PM
 */

#ifndef STATICLIB_HTTPCLIENT_ALL_REQUESTS_PAUSED_LATCH_HPP
#define	STATICLIB_HTTPCLIENT_ALL_REQUESTS_PAUSED_LATCH_HPP

#include <condition_variable>
#include <memory>
#include <mutex>

namespace staticlib {
namespace httpclient {

class all_requests_paused_latch : public std::enable_shared_from_this<all_requests_paused_latch> {
    std::mutex mutex;
    std::condition_variable cv;
    bool flag = false;
    
public:
    all_requests_paused_latch() { }
    
    all_requests_paused_latch(const all_requests_paused_latch&) = delete;
    
    all_requests_paused_latch& operator=(const all_requests_paused_latch&) = delete;
    
    void await() {
        std::unique_lock<std::mutex> guard{mutex};
        flag = true;
        cv.wait(guard, [this] {
            return !flag;
        });
    }
    
    void unlock() {
        bool was_locked = false;
        {
            std::lock_guard<std::mutex> guard{mutex};
            if (flag) {
                was_locked = true;
                flag = false;
            }
        }
        if (was_locked) {
            cv.notify_one();
        }
    }
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_ALL_REQUESTS_PAUSED_LATCH_HPP */

