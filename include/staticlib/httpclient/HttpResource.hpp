/* 
 * File:   HttpResource.hpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:43 AM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTPRESOURCE_HPP
#define	STATICLIB_HTTPCLIENT_HTTPRESOURCE_HPP

#include <ios>
#include <string>
#include <utility>
#include <vector>

#include "staticlib/pimpl.hpp"

#include "staticlib/httpclient/HttpClientException.hpp"

namespace staticlib {
namespace httpclient {

class HttpResource : public staticlib::pimpl::PimplObject {
friend class HttpSession;    
protected:
    /**
     * Implementation class
     */
    class Impl;

public:
    /**
     * PIMPL-specific constructor
     * 
     * @param pimpl impl object
     */
    PIMPL_CONSTRUCTOR(HttpResource)
            
    std::streamsize read(char* buffer, std::streamsize length);

private:
    /**
     * Private constructor for implementation details
     * 
     * @param impl_ptr implementation defined
     */
    HttpResource(/* CURLM */ void* multi_handle,
            const std::string& url,
            const std::string& method,
            const std::streambuf& data,
            const std::vector<std::pair<std::string, std::string>>& headers,
            const std::string& ssl_ca_file,
            const std::string& ssl_cert_file,
            const std::string& ssl_key_file,
            const std::string& ssl_key_passwd);
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTPRESOURCE_HPP */

