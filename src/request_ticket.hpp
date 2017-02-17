/* 
 * File:   request_ticket.hpp
 * Author: alex
 *
 * Created on February 15, 2017, 12:54 PM
 */

#ifndef STATICLIB_HTTPCLIENT_REQUEST_TICKET_HPP
#define	STATICLIB_HTTPCLIENT_REQUEST_TICKET_HPP

namespace staticlib {
namespace httpclient {

class request_ticket {
public:    
    std::string url;
    http_request_options options;
    std::unique_ptr<std::istream> post_data;
    std::shared_ptr<running_request_pipe> pipe;

    request_ticket() { }
    
    request_ticket(const std::string& url, http_request_options options,
            std::unique_ptr<std::istream>&& post_data,
            std::shared_ptr<running_request_pipe> pipe) :
    url(url.data(), url.length()),
    options(std::move(options)),
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

#endif	/* STATICLIB_HTTPCLIENT_REQUEST_TICKET_HPP */

