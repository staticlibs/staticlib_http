/* 
 * File:   HttpSession.hpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:42 AM
 */

#ifndef STATICLIB_HTTPCLIENT_HTTPSESSION_HPP
#define	STATICLIB_HTTPCLIENT_HTTPSESSION_HPP

#include <string>
#include <vector>
#include <utility>
#include <streambuf>
#include <sstream>

#ifdef STATICLIB_WITH_ICU
#include "unicode/unistr.h"    
#endif // STATICLIB_WITH_ICU

#include "staticlib/pimpl.hpp"

#include "staticlib/httpclient/HttpResource.hpp"

namespace staticlib {
namespace httpclient {

const std::istringstream EMPTY_STREAM{""};

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
    
    HttpSession(long conn_cache_size = 32);

#ifdef STATICLIB_WITH_ICU    
    HttpResource open_url(const icu::UnicodeString& url, 
            const icu::UnicodeString& method = "GET",
            const std::streambuf& data = *EMPTY_STREAM.rdbuf(),
            const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>& headers = {},
            const icu::UnicodeString& ssl_ca_file = "",
            const icu::UnicodeString& ssl_cert_file = "",
            const icu::UnicodeString& ssl_key_file = "",
            const icu::UnicodeString& ssl_key_passwd = "");
#else
    HttpResource open_url(const std::string& url,
            const std::string& method = "GET",
            const std::streambuf& data = *EMPTY_STREAM.rdbuf(),
            const std::vector<std::pair<std::string, std::string>>& headers = {},
            const std::string& ssl_ca_file = "",
            const std::string& ssl_cert_file = "",
            const std::string& ssl_key_file = "",
            const std::string& ssl_key_passwd = "");
#endif // STATICLIB_WITH_ICU
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTPSESSION_HPP */

