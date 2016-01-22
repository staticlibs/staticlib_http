/* 
 * File:   HttpSession_test.cpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:44 AM
 */

#include "staticlib/httpclient/HttpSession.hpp"

#include <iostream>

#include "staticlib/config/assert.hpp"

void test_not_leaked() {
    // check not leaked
    auto hs = staticlib::httpclient::HttpSession{};
    (void) hs;
}

int main() {
    try {
        test_not_leaked();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}

