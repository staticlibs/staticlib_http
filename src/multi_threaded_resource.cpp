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
 * File:   multi_threaded_resource.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:43 AM
 */

#include "multi_threaded_resource.hpp"

#include <cstring>
#include <chrono>
#include <ios>
#include <limits>
#include <memory>
#include <thread>

#include "staticlib/config.hpp"
#include "staticlib/concurrent.hpp"
#include "staticlib/pimpl/forward_macros.hpp"
#include "staticlib/utils.hpp"

#include "staticlib/http/resource_info.hpp"
#include "staticlib/http/http_exception.hpp"

#include "resource_impl.hpp"
#include "resource_params.hpp"

namespace staticlib {
namespace http {

namespace { // anonymous

using headers_type = const std::vector<std::pair<std::string, std::string>>&;

} // namespace


class multi_threaded_resource::impl : public resource::impl {
    uint64_t id;
    std::string url;

    mutable std::shared_ptr<running_request_pipe> pipe;
    mutable std::vector<std::pair<std::string, std::string>> headers;

    sl::concurrent::growing_buffer current_buf;
    size_t start_idx = 0;
    bool empty_response = false;

public:
    impl(uint64_t resource_id, resource_params&& params):
    resource::impl(),
    id(resource_id),
    url(params.url.data(), params.url.length()),
    pipe(std::move(params.pipe)) {
        // read first data chunk to make sure that status_code is ready
        this->empty_response = !pipe->receive_some_data(current_buf);
        if (pipe->has_errors()) {
            throw http_exception(TRACEMSG(pipe->get_error_message()));
        }
    }

    virtual std::streamsize read(resource&, sl::io::span<char> span) override {
        size_t avail = current_buf.size() - start_idx;
        if (avail > 0) {
            return read_from_current(span, avail);
        } else if (empty_response) {
            return std::char_traits<char>::eof();
        }
        start_idx = 0;
        bool success = pipe->receive_some_data(current_buf);
        if (pipe->has_errors()) {
            throw http_exception(TRACEMSG(pipe->get_error_message()));
        }
        if (success) {
            return read_from_current(span, current_buf.size());
        } else {
            return std::char_traits<char>::eof();
        }
    }

    virtual const std::string& get_url(const resource&) const override {
        return url;
    }

    virtual uint16_t get_status_code(const resource&) const override {
        return pipe->get_response_code();
    }

    virtual resource_info get_info(const resource&) const override {
        return pipe->get_resource_info();
    }

    virtual const std::vector<std::pair<std::string, std::string>>& get_headers(const resource&) const override {
        load_more_headers();
        return headers;
    }

    virtual const std::string& get_header(const resource&, const std::string& name) const override {
        // try cached first
        for (auto& en : headers) {
            if (name == en.first) {
                return en.second;
            }
        }
        // load more if available
        load_more_headers();
        for (auto& en : headers) {
            if (name == en.first) {
                return en.second;
            }
        }
        static std::string empty;
        return empty;
    }

    virtual bool connection_successful(const resource& frontend) const override {
        return get_status_code(frontend) > 0;
    }

    virtual uint64_t get_id(const resource&) const override {
        return id;
    }

    virtual const std::string& get_response_data_file(const resource&) const override {
        return sl::utils::empty_string();
    }

private:
    std::streamsize read_from_current(sl::io::span<char>& span, size_t avail) {
        size_t len = avail <= span.size() ? avail : span.size();
        std::memcpy(span.data(), current_buf.data() + start_idx, len);
        start_idx += len;
        return static_cast<std::streamsize> (len);
    }

    void load_more_headers() const {
        auto received = pipe->consume_received_headers();
        for (auto&& he : received) {
            headers.emplace_back(std::move(he));
        }
    }
};
PIMPL_FORWARD_CONSTRUCTOR(multi_threaded_resource, (uint64_t)(resource_params&&), (), http_exception)
PIMPL_FORWARD_METHOD(multi_threaded_resource, std::streamsize, read, (sl::io::span<char>), (), http_exception)
PIMPL_FORWARD_METHOD(multi_threaded_resource, const std::string&, get_url, (), (const), http_exception)
PIMPL_FORWARD_METHOD(multi_threaded_resource, uint16_t, get_status_code, (), (const), http_exception)
PIMPL_FORWARD_METHOD(multi_threaded_resource, resource_info, get_info, (), (const), http_exception)
PIMPL_FORWARD_METHOD(multi_threaded_resource, headers_type, get_headers, (), (const), http_exception)
PIMPL_FORWARD_METHOD(multi_threaded_resource, const std::string&, get_header, (const std::string&), (const), http_exception)
PIMPL_FORWARD_METHOD(multi_threaded_resource, bool, connection_successful, (), (const), http_exception)
PIMPL_FORWARD_METHOD(multi_threaded_resource, uint64_t, get_id, (), (const), http_exception)
PIMPL_FORWARD_METHOD(multi_threaded_resource, const std::string&, get_response_data_file, (), (const), http_exception)

} // namespace
}
