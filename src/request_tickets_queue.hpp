/* 
 * File:   request_tickets_queue.hpp
 * Author: alex
 *
 * Created on February 15, 2017, 12:57 PM
 */

#ifndef STATICLIB_HTTPCLIENT_REQUEST_TICKETS_QUEUE_HPP
#define	STATICLIB_HTTPCLIENT_REQUEST_TICKETS_QUEUE_HPP

#include "staticlib/containers.hpp"

#include "request_ticket.hpp"

namespace staticlib {
namespace httpclient {

using request_tickets_queue = staticlib::containers::synchronized_queue<request_ticket>;

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_REQUEST_TICKETS_QUEUE_HPP */

