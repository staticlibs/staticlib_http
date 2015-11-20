/* 
 * File:   HttpResource_test.cpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:44 AM
 */

#include <iostream>
#include <string>

#include "staticlib/utils.hpp"
#include "staticlib/utils/assert.hpp"

#include "staticlib/httpclient/HttpSession.hpp"
#include "staticlib/httpclient/HttpResource.hpp"

namespace hc = staticlib::httpclient;

void test_get() {
    hc::HttpSession session{};
    hc::HttpResource src = session.open_url_test("https://google.com/");
    std::string out{};
    out.resize(12);
    std::streamsize res = src.read(std::addressof(out.front()), 12);
    slassert(12 == res);
    slassert("<HTML><HEAD>" == out);
}

int main() {
    try {
        test_get();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}

