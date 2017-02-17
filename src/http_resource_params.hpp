/* 
 * File:   http_resource_params.hpp
 * Author: alex
 *
 * Created on February 16, 2017, 12:42 PM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTP_RESOURCE_PARAMS_HPP
#define	STATICLIB_HTTPCLIENT_HTTP_RESOURCE_PARAMS_HPP

#include <string>
#include <memory>

#include "running_request_pipe.hpp"

namespace staticlib {
namespace httpclient {

struct http_resource_params {
    const std::string& url;
    std::shared_ptr<running_request_pipe> pipe;
    
    http_resource_params(const std::string& url, std::shared_ptr<running_request_pipe>&& pipe) :
    url(url),
    pipe(std::move(pipe)) { }
    
    http_resource_params(const http_resource_params&) = delete;
    
    http_resource_params& operator=(const http_resource_params&) = delete;
    
    http_resource_params(http_resource_params&& other) :
    url(other.url),
    pipe(std::move(other.pipe)) { }
    
    http_resource_params& operator=(http_resource_params&&) = delete;
    
};

} // namespace
}


#endif	/* STATICLIB_HTTPCLIENT_HTTP_RESOURCE_PARAMS_HPP */

