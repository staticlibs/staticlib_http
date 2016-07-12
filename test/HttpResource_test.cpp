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

#ifdef STATICLIB_WITH_ICU
#include <unicode/unistr.h>
#endif // STATICLIB_WITH_ICU

#include "asio.hpp"

#include "staticlib/httpserver/tcp_connection.hpp"
#include "staticlib/httpserver/http_request.hpp"
#include "staticlib/httpserver/http_response_writer.hpp"
#include "staticlib/httpserver/http_server.hpp"

#include "staticlib/config/assert.hpp"
#include "staticlib/io.hpp"

#include "staticlib/httpclient/HttpSession.hpp"


namespace io = staticlib::io;
namespace sc = staticlib::config;
namespace hc = staticlib::httpclient;
namespace hs = staticlib::httpserver;

const uint16_t TCP_PORT = 8443;
#ifdef STATICLIB_WITH_ICU
const icu::UnicodeString URL = icu::UnicodeString() + "https://127.0.0.1:" + icu::UnicodeString::fromUTF8(sc::to_string(TCP_PORT)) + "/";
#else
const std::string URL = std::string() + "https://127.0.0.1:" + sc::to_string(TCP_PORT) + "/";
#endif // STATICLIB_WITH_ICU
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

void enrich_opts_ssl(hc::HttpRequestOptions& opts) {
    opts.sslcert_filename = CLIENT_CERT_PATH;
    opts.sslcertype = "PEM";
    opts.sslkey_filename = CLIENT_CERT_PATH;
    opts.ssl_key_type = "PEM";
    opts.ssl_keypasswd = "test";
}

void get_handler(hs::http_request_ptr& req, hs::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("GET" == req->get_header("X-Method"));
    auto writer = hs::http_response_writer::create(conn, req);
    writer->write(GET_RESPONSE);
    writer->send();
}

class PayloadReceiver {
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

void post_handler(hs::http_request_ptr& req, hs::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("POST" == req->get_header("X-Method"));
    auto ph = req->get_payload_handler<PayloadReceiver>();
    slassert(nullptr != ph);
    slassert(ph->is_received());
    auto writer = hs::http_response_writer::create(conn, req);
    writer->write(POST_RESPONSE);
    writer->send();
}

void put_handler(hs::http_request_ptr& req, hs::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("PUT" == req->get_header("X-Method"));
    auto ph = req->get_payload_handler<PayloadReceiver>();
    slassert(nullptr != ph);
    slassert(ph->is_received());
    auto writer = hs::http_response_writer::create(conn, req);
    writer->write(PUT_RESPONSE);
    writer->send();
}

void delete_handler(hs::http_request_ptr& req, hs::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("DELETE" == req->get_header("X-Method"));
    auto writer = hs::http_response_writer::create(conn, req);
    writer->write(DELETE_RESPONSE);
    writer->send();
}

void test_get() {
    // server
    hs::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("GET", "/", get_handler);
    server.start();
    // client
    try {
        hc::HttpSession session{};
        hc::HttpRequestOptions opts{};
        opts.headers = {{"User-Agent", "test"}, {"X-Method", "GET"}};
        enrich_opts_ssl(opts);
        opts.method = "GET";
    
        hc::HttpResource src = session.open_url(URL, opts);
        // check
        std::string out{};
        out.resize(GET_RESPONSE.size());
        std::streamsize res = io::read_all(src, std::addressof(out.front()), out.size());
        slassert(out.size() == static_cast<size_t>(res));
        slassert(GET_RESPONSE == out);
    } catch (const std::exception&) {
        server.stop(true);
        throw;
    }
    // stop server
    server.stop(true);
}

void test_post() {
    // server
    hs::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("POST", "/", post_handler);
    server.add_payload_handler("POST", "/", [](hs::http_request_ptr&) { return PayloadReceiver{}; });
    server.start();
    // client
    try {
        hc::HttpSession session{};
        hc::HttpRequestOptions opts{};
        opts.headers = {{"User-Agent", "test"}, {"X-Method", "POST"}};
        opts.method = "POST";
        enrich_opts_ssl(opts);
        io::string_source post_data{POSTPUT_DATA};
        hc::HttpResource src = session.open_url(URL, post_data, opts);
        // check
        std::string out{};
        out.resize(POST_RESPONSE.size());        
        std::streamsize res = io::read_all(src, std::addressof(out.front()), out.size());        
        slassert(out.size() == static_cast<size_t> (res));
        slassert(POST_RESPONSE == out);
    } catch (const std::exception&) {
        server.stop(true);
        throw;
    }
    // stop server
    server.stop(true);
}

void test_put() {
    // server
    hs::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("PUT", "/", put_handler);
    server.add_payload_handler("PUT", "/", [](hs::http_request_ptr&) {
        return PayloadReceiver{}; });
    server.start();
    // client
    try {
        hc::HttpSession session{};
        hc::HttpRequestOptions opts{};
        opts.headers = {{"User-Agent", "test"}, {"X-Method", "PUT"}};
        opts.method = "PUT";
        enrich_opts_ssl(opts);
        io::string_source post_data{POSTPUT_DATA};
        hc::HttpResource src = session.open_url(URL, std::move(post_data), opts);
        // check
        std::string out{};
        out.resize(PUT_RESPONSE.size());
        std::streamsize res = io::read_all(src, std::addressof(out.front()), out.size());
        slassert(out.size() == static_cast<size_t> (res));
        slassert(PUT_RESPONSE == out);
    } catch (const std::exception&) {
        server.stop(true);
        throw;
    }
    // stop server
    server.stop(true);
}

void test_delete() {
    // server
    hs::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("DELETE", "/", delete_handler);
    server.start();
    // client
    try {
        hc::HttpSession session{};
        hc::HttpRequestOptions opts{};
        opts.headers = {{"User-Agent", "test"}, {"X-Method", "DELETE"}};
        enrich_opts_ssl(opts);
        opts.method = "DELETE";

        hc::HttpResource src = session.open_url(URL, opts);
        // check
        std::string out{};
        out.resize(DELETE_RESPONSE.size());
        std::streamsize res = io::read_all(src, std::addressof(out.front()), out.size());
        slassert(out.size() == static_cast<size_t> (res));
        slassert(DELETE_RESPONSE == out);
    } catch (const std::exception&) {
        server.stop(true);
        throw;
    }
    // stop server
    server.stop(true);
}

int main() {
    try {
        //auto start = std::chrono::system_clock::now();
        test_get();
        test_post();
        test_put();
        test_delete();
        //auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
        //std::cout << elapsed.count() << std::endl;
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}

