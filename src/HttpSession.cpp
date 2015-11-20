/* 
 * File:   HttpSession.cpp
 * Author: alex
 * 
 * Created on November 20, 2015, 8:42 AM
 */

#include <memory>

#include "curl/curl.h"

#include "staticlib/utils/tracemsg.hpp"
#include "staticlib/pimpl/pimpl_forward_macros.hpp"

#include "staticlib/httpclient/HttpClientException.hpp"
#include "staticlib/httpclient/HttpResource.hpp"
#include "staticlib/httpclient/HttpSession.hpp"

namespace staticlib {
namespace httpclient {

namespace { // anonymous

class CurlMultiDeleter {
public:
    void operator()(CURLM* curlm) {
        curl_multi_cleanup(curlm);
    }   
};

} // namespace

class HttpSession::Impl : public staticlib::pimpl::PimplObject::Impl {
    std::unique_ptr<CURLM, CurlMultiDeleter> handle;

public:
    Impl() :
    handle(curl_multi_init(), CurlMultiDeleter{}) { 
        if (nullptr == handle.get()) throw HttpClientException(TRACEMSG("Error initialising cURL multi handle"));
    }
    
    HttpResource open_url_test(HttpSession&, const std::string& url) {
        return HttpResource(handle.get(), url);
    }
    
};
PIMPL_FORWARD_CONSTRUCTOR(HttpSession, (), (), HttpClientException) 
PIMPL_FORWARD_METHOD(HttpSession, HttpResource, open_url_test, (const std::string&), (), HttpClientException)


} // namespace
}
