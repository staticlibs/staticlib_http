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
 * File:   polling_test.cpp
 * Author: alex
 *
 * Created on January 17, 2021, 1:59 PM
 */

#include "staticlib/http/resource.hpp"

#include <cstdint>
#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "asio.hpp"

#include "staticlib/pion.hpp"

#include "staticlib/config/assert.hpp"
#include "staticlib/io.hpp"
#include "staticlib/tinydir.hpp"

#include "staticlib/http/polling_session.hpp"

const uint16_t TCP_PORT = 8443;
const std::string URL = std::string() + "https://127.0.0.1:" + sl::support::to_string(TCP_PORT) + "/";
const std::string GET_RESPONSE = "Hello from GET\n";
const std::string POSTPUT_DATA = "Hello to POST\n";
const std::string POST_RESPONSE = "Hello from POST\n";
const std::string PUT_RESPONSE = "Hello from PUT\n";
const std::string DELETE_RESPONSE = "Hello from DELETE\n";
const std::string SERVER_CERT_PATH = "../test/certificates/server/localhost.pem";
const std::string CLIENT_CERT_PATH = "../test/certificates/client/testclient.pem";
const std::string CA_PATH = "../test/certificates/server/staticlibs_test_ca.cer";

std::string pwdcb(std::size_t, asio::ssl::context::password_purpose) {
    return "test";
};

bool verifier(bool, asio::ssl::verify_context&) {
    return true;
};

void enrich_opts_ssl(sl::http::request_options& opts) {
    opts.sslcert_filename = CLIENT_CERT_PATH;
    opts.sslcertype = "PEM";
    opts.sslkey_filename = CLIENT_CERT_PATH;
    opts.ssl_key_type = "PEM";
    opts.ssl_keypasswd = "test";
}

void get_handler(sl::pion::http_request_ptr req, sl::pion::response_writer_ptr resp) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("GET" == req->get_header("X-Method"));
    resp->write(GET_RESPONSE);
    resp->send(std::move(resp));
}

class payload_receiver {
    bool received;
public:
    void operator()(const char* s, size_t n) {
        slassert(POSTPUT_DATA == std::string(s, n));
        received = true;
    }

    bool is_received() {
        return received;
    }
};

void post_handler(sl::pion::http_request_ptr req, sl::pion::response_writer_ptr resp) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("POST" == req->get_header("X-Method"));
    auto ph = req->get_payload_handler<payload_receiver>();
    slassert(nullptr != ph);
    slassert(ph->is_received());
    resp->write(POST_RESPONSE);
    resp->send(std::move(resp));
}

void enqueue_get(sl::http::polling_session& session) {
    // enqueue request
    sl::http::request_options opts{};
    opts.headers = {{"User-Agent", "test"}, {"X-Method", "GET"}};
    enrich_opts_ssl(opts);
    opts.method = "GET";
    sl::http::resource src = session.open_url(URL + "get", opts);

    // check empty
    auto empty = std::string();
    sl::io::read_all(src, empty);
    slassert(empty.empty());
}

void enqueue_post(sl::http::polling_session& session) {
    // enqueue request
    sl::http::request_options opts{};
    opts.headers = {{"User-Agent", "test"}, {"X-Method", "POST"}};
    enrich_opts_ssl(opts);
    sl::io::string_source post_data{POSTPUT_DATA};
    opts.method = "POST";
    sl::http::resource src = session.open_url(URL + "post", std::move(post_data), opts);

    // check empty
    auto empty = std::string();
    sl::io::read_all(src, empty);
    slassert(empty.empty());
}

std::vector<sl::http::resource> poll(sl::http::polling_session& session,
        uint32_t count, uint32_t max_count) {
    auto vec = std::vector<sl::http::resource>();
    for (uint32_t i = 0; i < max_count; i++) {
        //auto start = std::chrono::system_clock::now();
        auto completed = session.poll();
        //auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
        //std::cout << i << std::endl;
        //std::cout << completed.size() << std::endl;
        //std::cout << "millis elapsed: " << elapsed.count() << std::endl;
        for (auto&& res : completed) {
            vec.emplace_back(std::move(res));
        }
        if (vec.size() >= count) break;
    }
    return vec;
}

void test_simple() {
    // server
    //sl::pion::http_server server(2, TCP_PORT);
    sl::pion::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), 10000, SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("GET", "/get", get_handler);
    server.add_handler("POST", "/post", post_handler);
    server.add_payload_handler("POST", "/post", [](sl::pion::http_request_ptr&) { return payload_receiver{}; });
    server.start();
    try {
        // session
        auto session = sl::http::polling_session();

        { // get
            enqueue_get(session);
            auto vec = poll(session, 1, 1024);

            // check response
            slassert(1 == vec.size());
            auto res = std::move(vec.at(0));
            slassert(200 == res.get_status_code());
            slassert(sl::support::to_string(GET_RESPONSE.length()) == res.get_header("Content-Length"));
            auto data = std::string();
            data.resize(GET_RESPONSE.length());
            auto read = sl::io::read_all(res, data);
            slassert(GET_RESPONSE.length() == read);
            slassert(GET_RESPONSE == data);
        }

        { // post
            enqueue_post(session);
            auto vec = poll(session, 1, 1024);

            // check response
            slassert(1 == vec.size());
            auto res = std::move(vec.at(0));
            slassert(200 == res.get_status_code());
            slassert(sl::support::to_string(POST_RESPONSE.length()) == res.get_header("Content-Length"));
            auto data = std::string();
            data.resize(POST_RESPONSE.length());
            auto read = sl::io::read_all(res, data);
            slassert(POST_RESPONSE.length() == read);
            slassert(POST_RESPONSE == data);
        }

    } catch (const std::exception&) {
        server.stop(true);
        throw;
    }
    // stop server
    server.stop(true);
}

void test_parallel() {
    const uint32_t count = 256;
    // server
    //sl::pion::http_server server(2, TCP_PORT);
    sl::pion::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), 10000, SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("GET", "/get", get_handler);
    server.add_handler("POST", "/post", post_handler);
    server.add_payload_handler("POST", "/post", [](sl::pion::http_request_ptr&) { return payload_receiver{}; });
    server.start();
    try {
        // session
        auto session = sl::http::polling_session();

        for (uint32_t i = 0; i < count/2; i++) {
            enqueue_get(session);
            enqueue_post(session);
        }
        auto vec = poll(session, count, 1024);

        // check responses
        slassert(count == vec.size());
        for(auto& res : vec) {
            slassert(200 == res.get_status_code());
            if (sl::utils::ends_with(res.get_url(), "get")) {
                slassert(sl::support::to_string(GET_RESPONSE.length()) == res.get_header("Content-Length"));
                auto data = std::string();
                data.resize(GET_RESPONSE.length());
                auto read = sl::io::read_all(res, data);
                slassert(GET_RESPONSE.length() == read);
                slassert(GET_RESPONSE == data);
            } else {
                slassert(sl::support::to_string(POST_RESPONSE.length()) == res.get_header("Content-Length"));
                auto data = std::string();
                data.resize(POST_RESPONSE.length());
                auto read = sl::io::read_all(res, data);
                slassert(POST_RESPONSE.length() == read);
                slassert(POST_RESPONSE == data);
            }
        }

    } catch (const std::exception&) {
        server.stop(true);
        throw;
    }
    // stop server
    server.stop(true);
}


int main() {
    try {
        test_simple();
        // too slow under valgrind
        //test_parallel();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}
