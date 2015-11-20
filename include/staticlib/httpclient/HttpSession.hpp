/* 
 * File:   HttpSession.hpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:42 AM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTPSESSION_HPP
#define	STATICLIB_HTTPCLIENT_HTTPSESSION_HPP

#include <string>

#include "staticlib/pimpl.hpp"

#include "staticlib/httpclient/HttpResource.hpp"

namespace staticlib {
namespace httpclient {

class HttpSession : public staticlib::pimpl::PimplObject {
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
    PIMPL_CONSTRUCTOR(HttpSession)
    
    HttpSession();
    
    HttpResource open_url_test(const std::string& url);
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTPSESSION_HPP */

