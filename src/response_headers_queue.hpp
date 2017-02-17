/* 
 * File:   response_headers_queue.hpp
 * Author: alex
 *
 * Created on February 14, 2017, 8:56 PM
 */

#ifndef STATICLIB_HTTPCLIENT_RESPONSE_HEADERS_QUEUE_HPP
#define	STATICLIB_HTTPCLIENT_RESPONSE_HEADERS_QUEUE_HPP

#include <string>
#include <vector>

#include "staticlib/containers.hpp"

namespace staticlib {
namespace httpclient {

using response_headers_queue = staticlib::containers::synchronized_queue<std::pair<std::string, std::string>>;

} // namespace
}


#endif	/* STATICLIB_HTTPCLIENT_RESPONSE_HEADERS_QUEUE_HPP */

