/* 
 * File:   HttpResource_test.cpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:44 AM
 */

#include "staticlib/httpclient/HttpResource.hpp"

#include <array>
#include <iostream>
#include <string>

#include "staticlib/config/assert.hpp"
#include "staticlib/io.hpp"

#include "staticlib/httpclient/HttpSession.hpp"


namespace hc = staticlib::httpclient;
namespace io = staticlib::io;

void test_get() {
    hc::HttpSession session{};
    hc::HttpResource src = session.open_url("https://google.com/");
    std::string out{};
    out.resize(12);
    std::streamsize res = src.read(std::addressof(out.front()), 12);
    slassert(12 == res);
    slassert("<HTML><HEAD>" == out);
    
//    io::string_sink sink{};
//    std::array<char, 4096> buf;
//    auto res_all = io::copy_all(src, sink, buf.data(), buf.size());
//    std::cout << res_all << std::endl;
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

