/* 
 * File:   HttpSession.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:42 AM
 */

#include <memory>
#include <sstream>

#include "curl/curl.h"

#include "staticlib/utils/tracemsg.hpp"
#include "staticlib/pimpl/pimpl_forward_macros.hpp"

#include "staticlib/httpclient/HttpClientException.hpp"
#include "staticlib/httpclient/HttpResource.hpp"
#include "staticlib/httpclient/HttpSession.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

#ifdef STATICLIB_WITH_ICU
typedef const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>& headers_type;
#else
typedef const std::vector<std::pair<std::string, std::string>>& headers_type;
#endif // STATICLIB_WITH_ICU

class CurlMultiDeleter {
public:
    void operator()(CURLM* curlm) {
        curl_multi_cleanup(curlm);
    }   
};

} // namespace

class HttpSession::Impl : public staticlib::pimpl::PimplObject::Impl {
    std::unique_ptr<CURLM, CurlMultiDeleter> handle;
    std::istringstream empty_stream;

public:
    Impl() :
    handle(curl_multi_init(), CurlMultiDeleter{}) { 
        if (nullptr == handle.get()) throw HttpClientException(TRACEMSG("Error initialising cURL multi handle"));
    }
    
    ~Impl() STATICLIB_NOEXCEPT { }
    
#ifdef STATICLIB_WITH_ICU
    HttpResource open_get(HttpSession&, const icu::UnicodeString& url,
            const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>& headers,
            const icu::UnicodeString& ssl_ca_file,
            const icu::UnicodeString& ssl_cert_file,
            const icu::UnicodeString& ssl_key_file,
            const icu::UnicodeString& ssl_key_passwd) {
        return HttpResource(handle.get(), "GET", to_utf8(url), *empty_stream.rdbuf(), 
                headers_to_utf8(headers), to_utf8(ssl_ca_file), to_utf8(ssl_cert_file), 
                to_utf8(ssl_key_file), to_utf8(ssl_key_passwd));        
    }
#else
    HttpResource open_get(HttpSession&, const std::string& url,
            const std::vector<std::pair<std::string, std::string>>& headers,
            const std::string& ssl_ca_file,
            const std::string& ssl_cert_file,
            const std::string& ssl_key_file,
            const std::string& ssl_key_passwd) {
        return HttpResource(handle.get(), "GET", url, *empty_stream.rdbuf(), headers,
                ssl_ca_file, ssl_cert_file, ssl_key_file, ssl_key_passwd);
    }
#endif // STATICLIB_WITH_ICU

#ifdef STATICLIB_WITH_ICU    
    HttpResource open_post(HttpSession&, const icu::UnicodeString& url, const std::streambuf& data,
            const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>& headers,
            const icu::UnicodeString& ssl_ca_file,
            const icu::UnicodeString& ssl_cert_file,
            const icu::UnicodeString& ssl_key_file,
            const icu::UnicodeString& ssl_key_passwd) {
        return HttpResource(handle.get(), "POST", to_utf8(url), data,
                headers_to_utf8(headers), to_utf8(ssl_ca_file), to_utf8(ssl_cert_file), 
                to_utf8(ssl_key_file), to_utf8(ssl_key_passwd));
    }
#else
    HttpResource open_post(HttpSession&, const std::string& url, const std::streambuf& data,
            const std::vector<std::pair<std::string, std::string>>& headers,
            const std::string& ssl_ca_file,
            const std::string& ssl_cert_file,
            const std::string& ssl_key_file,
            const std::string& ssl_key_passwd) {
        return HttpResource(handle.get(), "POST", url, data, headers,
                ssl_ca_file, ssl_cert_file, ssl_key_file, ssl_key_passwd);
    }    
#endif // STATICLIB_WITH_ICU

#ifdef STATICLIB_WITH_ICU
    HttpResource open_put(HttpSession&, const icu::UnicodeString& url, const std::streambuf& data,
            const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>& headers,
            const icu::UnicodeString& ssl_ca_file,
            const icu::UnicodeString& ssl_cert_file,
            const icu::UnicodeString& ssl_key_file,
            const icu::UnicodeString& ssl_key_passwd) {
        return HttpResource(handle.get(), "PUT", to_utf8(url), data,
                headers_to_utf8(headers), to_utf8(ssl_ca_file), to_utf8(ssl_cert_file), 
                to_utf8(ssl_key_file), to_utf8(ssl_key_passwd));
    }
#else
    HttpResource open_put(HttpSession&, const std::string& url, const std::streambuf& data,
            const std::vector<std::pair<std::string, std::string>>& headers,
            const std::string& ssl_ca_file,
            const std::string& ssl_cert_file,
            const std::string& ssl_key_file,
            const std::string& ssl_key_passwd) {
        return HttpResource(handle.get(), "PUT", url, data, headers,
                ssl_ca_file, ssl_cert_file, ssl_key_file, ssl_key_passwd);
    }
#endif // STATICLIB_WITH_ICU

#ifdef STATICLIB_WITH_ICU
    HttpResource open_delete(HttpSession&, const icu::UnicodeString& url,
            const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>& headers,
            const icu::UnicodeString& ssl_ca_file,
            const icu::UnicodeString& ssl_cert_file,
            const icu::UnicodeString& ssl_key_file,
            const icu::UnicodeString& ssl_key_passwd) {
        return HttpResource(handle.get(), "DELETE", to_utf8(url), *empty_stream.rdbuf(),
                headers_to_utf8(headers), to_utf8(ssl_ca_file), to_utf8(ssl_cert_file), 
                to_utf8(ssl_key_file), to_utf8(ssl_key_passwd));
    }
#else
    HttpResource open_delete(HttpSession&, const std::string& url,
            const std::vector<std::pair<std::string, std::string>>& headers,
            const std::string& ssl_ca_file,
            const std::string& ssl_cert_file,
            const std::string& ssl_key_file,
            const std::string& ssl_key_passwd) {
        return HttpResource(handle.get(), "DELETE", url, *empty_stream.rdbuf(), headers,
                ssl_ca_file, ssl_cert_file, ssl_key_file, ssl_key_passwd);
    }
#endif // STATICLIB_WITH_ICU    

private:
#ifdef STATICLIB_WITH_ICU
    std::string to_utf8(const icu::UnicodeString& str) {
        std::string bytes{};
        str.toUTF8String(bytes);
        return bytes;
    }

    std::vector<std::pair<std::string, std::string>> 
    headers_to_utf8(const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>& headers) {
        std::vector<std::pair<std::string, std::string>> res{};
        for (auto& en : headers) {
            res.emplace_back(to_utf8(en.first), to_utf8(en.second));
        }
        return res;
    }
#endif // STATICLIB_WITH_ICU
    
};
PIMPL_FORWARD_CONSTRUCTOR(HttpSession, (), (), HttpClientException)
#ifdef STATICLIB_WITH_ICU        
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_get,
        (const icu::UnicodeString&)(headers_type)(const icu::UnicodeString&)(const icu::UnicodeString&)(const icu::UnicodeString&)(const icu::UnicodeString&),
        (), HttpClientException)
#else
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_get,
        (const std::string&)(headers_type) (const std::string&)(const std::string&)(const std::string&)(const std::string&),
        (), HttpClientException)        
#endif // STATICLIB_WITH_ICU        
#ifdef STATICLIB_WITH_ICU        
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_post,
        (const icu::UnicodeString&)(const std::streambuf&)(headers_type) (const icu::UnicodeString&)(const icu::UnicodeString&)(const icu::UnicodeString&)(const icu::UnicodeString&),
        (), HttpClientException)
#else
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_post,
        (const std::string&)(const std::streambuf&)(headers_type) (const std::string&)(const std::string&)(const std::string&)(const std::string&),
        (), HttpClientException)        
#endif // STATICLIB_WITH_ICU        
#ifdef STATICLIB_WITH_ICU        
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_put,
        (const icu::UnicodeString&)(const std::streambuf&)(headers_type) (const icu::UnicodeString&)(const icu::UnicodeString&)(const icu::UnicodeString&)(const icu::UnicodeString&),
        (), HttpClientException)
#else
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_put,
        (const std::string&)(const std::streambuf&)(headers_type) (const std::string&)(const std::string&)(const std::string&)(const std::string&),
        (), HttpClientException)
#endif // STATICLIB_WITH_ICU        
#ifdef STATICLIB_WITH_ICU
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_delete,
        (const icu::UnicodeString&)(headers_type) (const icu::UnicodeString&)(const icu::UnicodeString&)(const icu::UnicodeString&)(const icu::UnicodeString&),
        (), HttpClientException)
#else
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_delete,
        (const std::string&)(headers_type) (const std::string&)(const std::string&)(const std::string&)(const std::string&),
        (), HttpClientException)        
#endif // STATICLIB_WITH_ICU        



} // namespace
}
