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
 * File:   HttpResource_test.cpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:44 AM
 */

#include "staticlib/httpclient/HttpResource.hpp"

#include <array>
#include <iostream>
#include <string>
#include <cstdint>

#include "asio.hpp"

#include "pion/tcp/connection.hpp"
#include "pion/http/request.hpp"
#include "pion/http/response_writer.hpp"
#include "pion/http/streaming_server.hpp"

#include "staticlib/config/assert.hpp"
#include "staticlib/io.hpp"

#include "staticlib/httpclient/HttpSession.hpp"


namespace io = staticlib::io;
namespace sc = staticlib::config;
namespace hc = staticlib::httpclient;

const uint16_t TCP_PORT = 8443;
const std::string GET_RESPONSE =    "Hello from GET\n   ";
const std::string POST_RESPONSE =   "Hello from POST\n  ";
const std::string PUT_RESPONSE =    "Hello from PUT\n   ";
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

void get_handler(pion::http::request_ptr& req, pion::tcp::connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("GET" == req->get_header("X-Method"));
    auto finfun = std::bind(&pion::tcp::connection::finish, conn);
    auto writer = pion::http::response_writer::create(conn, *req, finfun);
    writer->write(GET_RESPONSE);
    writer->send();
}

void test_get() {
    // server
    pion::http::streaming_server server(1, TCP_PORT, asio::ip::address_v4::any(), SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("GET", "/", get_handler);
    server.start();
    // client
    {
        hc::HttpSession session{};
        hc::HttpRequestOptions opts{};
        opts.headers = {{"User-Agent", "test"}, {"X-Method", "GET"}};
        opts.sslcert_filename = CLIENT_CERT_PATH;
        opts.sslcertype = "PEM";
        opts.sslkey_filename = CLIENT_CERT_PATH;
        opts.ssl_key_type = "PEM";
        opts.ssl_keypasswd = "test";
    
        hc::HttpResource src = session.open_url(std::string() + "https://127.0.0.1:" + sc::to_string(TCP_PORT) + "/", opts);
        // check
        std::string out{};
        out.resize(GET_RESPONSE.size());
        std::streamsize res = io::read_all(src, std::addressof(out.front()), out.size());
        slassert(out.size() == static_cast<size_t>(res));
        slassert(GET_RESPONSE == out);
    }
    // stop server
    server.stop(true);
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

