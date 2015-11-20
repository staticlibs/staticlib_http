/* 
 * File:   HttpSession_test.cpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:44 AM
 */

#include "staticlib/httpclient/HttpSession.hpp"

int main() {
    // check not leaked
    auto hs = staticlib::httpclient::HttpSession{};
    (void) hs;

    return 0;
}

