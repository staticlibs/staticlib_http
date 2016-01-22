/* 
 * File:   HttpSession.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:42 AM
 */

#include "staticlib/httpclient/HttpSession.hpp"

#include <memory>

#include "curl/curl.h"


#include "staticlib/config.hpp"
#include "staticlib/pimpl/pimpl_forward_macros.hpp"

#include "staticlib/httpclient/HttpResource.hpp"


namespace staticlib {
namespace httpclient {

namespace { // anonymous

#ifdef STATICLIB_WITH_ICU
typedef const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>& headers_type;
#else
typedef const std::vector<std::pair<std::string, std::string>>& headers_type;
#endif // STATICLIB_WITH_ICU

namespace sc = staticlib::config;

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
    Impl(long conn_cache_size) :
    handle(curl_multi_init(), CurlMultiDeleter{}) { 
        if (nullptr == handle.get()) throw HttpClientException(TRACEMSG("Error initializing cURL multi handle"));
        CURLMcode err = curl_multi_setopt(handle.get(), CURLMOPT_MAXCONNECTS, conn_cache_size);
        if (err != CURLM_OK) throw HttpClientException(TRACEMSG(std::string() +
                "CURLMOPT_MAXCONNECTS error: [" + sc::to_string(err) + "]," +
                " size: [" + sc::to_string(conn_cache_size) + "]"));
    }
    
    ~Impl() STATICLIB_NOEXCEPT { }
    
#ifdef STATICLIB_WITH_ICU    
    HttpResource open_url(HttpSession&, const icu::UnicodeString& url, 
            const icu::UnicodeString& method,
            const std::streambuf& data,
            const std::vector<std::pair<icu::UnicodeString, icu::UnicodeString>>& headers,
            const icu::UnicodeString& ssl_ca_file,
            const icu::UnicodeString& ssl_cert_file,
            const icu::UnicodeString& ssl_key_file,
            const icu::UnicodeString& ssl_key_passwd) {
        return HttpResource(handle.get(), to_utf8(url), to_utf8(method), data,
                headers_to_utf8(headers), to_utf8(ssl_ca_file), to_utf8(ssl_cert_file), 
                to_utf8(ssl_key_file), to_utf8(ssl_key_passwd));
    }
#else
    HttpResource open_url(HttpSession&, const std::string& url, 
            const std::string& method,
            const std::streambuf& data,
            const std::vector<std::pair<std::string, std::string>>& headers,
            const std::string& ssl_ca_file,
            const std::string& ssl_cert_file,
            const std::string& ssl_key_file,
            const std::string& ssl_key_passwd) {
        return HttpResource(handle.get(), "POST", url, data, headers,
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
PIMPL_FORWARD_CONSTRUCTOR(HttpSession, (long), (), HttpClientException)
#ifdef STATICLIB_WITH_ICU        
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_url,
        (const icu::UnicodeString&)
        (const icu::UnicodeString&)
        (const std::streambuf&)
        (headers_type)
        (const icu::UnicodeString&)
        (const icu::UnicodeString&)
        (const icu::UnicodeString&)
        (const icu::UnicodeString&),
        (), HttpClientException)
#else
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_url,
        (const std::string&)
        (const std::string&)
        (const std::streambuf&)
        (headers_type)
        (const std::string&)
        (const std::string&)
        (const std::string&)
        (const std::string&),
        (), HttpClientException)        
#endif // STATICLIB_WITH_ICU        


} // namespace
}
