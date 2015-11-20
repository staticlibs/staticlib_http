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

#ifdef STATICLIB_WITH_ICU
#include "unicode/unistr.h"    
#endif // STATICLIB_WITH_ICU

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
    
    HttpResource open_get(const std::string& url, 
            const std::vector<std::pair<std::string, std::string>>& headers = {},
            const std::string& ssl_ca_file = "",
            const std::string& ssl_cert_file = "",
            const std::string& ssl_key_file = "",
            const std::string& ssl_key_passwd = "");

#ifdef STATICLIB_WITH_ICU
    HttpResource open_get(const icu::UnicodeString& url,
            const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>&headers = {},
            const icu::UnicodeString& ssl_cert_file = "",
            const icu::UnicodeString& ssl_key_file = "",
            const icu::UnicodeString& ssl_key_passwd = "");
#endif // STATICLIB_WITH_ICU
            
    HttpResource open_post(const std::string& url, const std::streambuf& data,
            const std::vector<std::pair<std::string, std::string>>& headers = {},
            const std::string& ssl_ca_file = "",
            const std::string& ssl_cert_file = "",
            const std::string& ssl_key_file = "",
            const std::string& ssl_key_passwd = "");

#ifdef STATICLIB_WITH_ICU    
    HttpResource open_post(const icu::UnicodeString& url, const std::streambuf& data,
            const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>&headers = {},
            const icu::UnicodeString& ssl_cert_file = "",
            const icu::UnicodeString& ssl_key_file = "",
            const icu::UnicodeString& ssl_key_passwd = "");
#endif // STATICLIB_WITH_ICU
    
    HttpResource open_put(const std::string& url, const std::streambuf& data,
            const std::vector<std::pair<std::string, std::string>>& headers = {},
            const std::string& ssl_ca_file = "",
            const std::string& ssl_cert_file = "",
            const std::string& ssl_key_file = "",
            const std::string& ssl_key_passwd = "");

#ifdef STATICLIB_WITH_ICU
    HttpResource open_put(const icu::UnicodeString& url, const std::streambuf& data,
            const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>&headers = {},
            const icu::UnicodeString& ssl_cert_file = "",
            const icu::UnicodeString& ssl_key_file = "",
            const icu::UnicodeString& ssl_key_passwd = "");
#endif // STATICLIB_WITH_ICU

    HttpResource open_delete(const std::string& url,
            const std::vector<std::pair<std::string, std::string>>& headers = {},
            const std::string& ssl_ca_file = "",
            const std::string& ssl_cert_file = "",
            const std::string& ssl_key_file = "",
            const std::string& ssl_key_passwd = "");

#ifdef STATICLIB_WITH_ICU    
    HttpResource open_delete(const icu::UnicodeString& url,
            const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>&headers = {},
            const icu::UnicodeString& ssl_cert_file = "",
            const icu::UnicodeString& ssl_key_file = "",
            const icu::UnicodeString& ssl_key_passwd = "");
#endif // STATICLIB_WITH_ICU    
    
};

} // namespace
}

#endif	/* STATICLIB_HTTPCLIENT_HTTPSESSION_HPP */

