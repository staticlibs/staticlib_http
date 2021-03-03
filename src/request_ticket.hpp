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
 * File:   request_ticket.hpp
 * Author: alex
 *
 * Created on February 15, 2017, 12:54 PM
 */

#ifndef STATICLIB_HTTP_REQUEST_TICKET_HPP
#define STATICLIB_HTTP_REQUEST_TICKET_HPP

namespace staticlib {
namespace http {

class request_ticket {
public:
    std::string url;
    request_options options;
    std::unique_ptr<std::istream> post_data;
    std::shared_ptr<running_request_pipe> pipe;

    request_ticket() { }

    request_ticket(const std::string& url, const request_options& options,
            std::unique_ptr<std::istream>&& post_data,
            std::shared_ptr<running_request_pipe> pipe) :
    url(url.data(), url.length()),
    options(options),
    post_data(std::move(post_data)),
    pipe(std::move(pipe)) { }

    request_ticket(const request_ticket&) = delete;

    request_ticket& operator=(const request_ticket&) = delete;

    request_ticket(request_ticket&& other) :
    url(std::move(other.url)),
    options(std::move(other.options)),
    post_data(std::move(other.post_data)),
    pipe(std::move(other.pipe)) { }

    request_ticket& operator=(request_ticket&& other) {
        url = std::move(other.url);
        options = std::move(other.options);
        post_data = std::move(other.post_data);
        pipe = std::move(other.pipe);
        return *this;
    }

};

} // namespace
}

#endif /* STATICLIB_HTTP_REQUEST_TICKET_HPP */

