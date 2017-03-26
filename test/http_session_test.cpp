/*
 * Copyright 2016, alex at staticlibs.net
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * File:   http_session_test.cpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:44 AM
 */

#include "staticlib/httpclient/multi_threaded_http_session.hpp"
#include "staticlib/httpclient/single_threaded_http_session.hpp"

#include <iostream>

#include "staticlib/config/assert.hpp"

void test_not_leaked() {
    // check not leaked
    auto mt = staticlib::httpclient::multi_threaded_http_session{};
    (void) mt;
    auto st = staticlib::httpclient::single_threaded_http_session{};
    (void) st;
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

