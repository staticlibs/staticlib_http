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
 * File:   http_resource_test.cpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:44 AM
 */

#include "staticlib/httpclient/http_resource.hpp"

#include <cstdint>
#include <array>
#include <iostream>
#include <string>
#include <thread>

#include "asio.hpp"

#include "staticlib/httpserver/tcp_connection.hpp"
#include "staticlib/httpserver/http_request.hpp"
#include "staticlib/httpserver/http_response_writer.hpp"
#include "staticlib/httpserver/http_server.hpp"

#include "staticlib/config/assert.hpp"
#include "staticlib/crypto.hpp"
#include "staticlib/io.hpp"
#include "staticlib/tinydir.hpp"

#include "staticlib/httpclient/http_session.hpp"


namespace io = staticlib::io;
namespace sc = staticlib::config;
namespace hc = staticlib::httpclient;
namespace hs = staticlib::httpserver;
namespace cr = staticlib::crypto;
namespace st = staticlib::tinydir;

const uint16_t TCP_PORT = 8443;
const std::string URL = std::string() + "https://127.0.0.1:" + sc::to_string(TCP_PORT) + "/";
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

void enrich_opts_ssl(hc::http_request_options& opts) {
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

class response_sender : public std::enable_shared_from_this<response_sender> {
    std::mutex mutex;
    hs::http_response_writer_ptr writer;
    std::unique_ptr<std::streambuf> stream;
    std::array<char, 4096> buf;

public:
    response_sender(hs::http_response_writer_ptr writer,
            std::unique_ptr<std::streambuf> stream) :
    writer(writer),
    stream(std::move(stream)) { }

    void send() {
        asio::error_code ec;
        handle_write(ec, 0);
    }

    void handle_write(const asio::error_code& ec, std::size_t /* bytes_written */) {
        std::lock_guard<std::mutex> lock{mutex};
        if (!ec) {
            auto src = io::streambuf_source(stream.get());
            size_t read = io::read_all(src, buf);
            writer->clear();
            if (read > 0) {
                writer->write_no_copy(buf.data(), read);
                auto self = shared_from_this();
                writer->send_chunk([self](const asio::error_code& ec, size_t bt) {
                    self->handle_write(ec, bt);
                });
            } else {
                writer->send_final_chunk();
            }
        } else {
            // make sure it will get closed
            writer->get_connection()->set_lifecycle(hs::tcp_connection::LIFECYCLE_CLOSE);
        }
    }
};

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

void post_handler(hs::http_request_ptr& req, hs::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("POST" == req->get_header("X-Method"));
    auto ph = req->get_payload_handler<payload_receiver>();
    slassert(nullptr != ph);
    slassert(ph->is_received());
    auto writer = hs::http_response_writer::create(conn, req);
    writer->write(POST_RESPONSE);
    writer->send();
}

void put_handler(hs::http_request_ptr& req, hs::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("PUT" == req->get_header("X-Method"));
    auto ph = req->get_payload_handler<payload_receiver>();
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

void large_handler(hs::http_request_ptr& req, hs::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    auto writer = hs::http_response_writer::create(conn, req);
    auto src = io::make_unbuffered_istreambuf_ptr(st::file_source("/home/alex/vbox/hd/Android-x86_4.4_r4.7z"));
    auto sender = std::make_shared<response_sender>(writer, std::move(src));
    sender->send();
}

void test_get() {
    // server
    hs::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("GET", "/", get_handler);
    server.start();
    // client
    try {
        hc::http_session session{};
        hc::http_request_options opts{};
        opts.headers = {{"User-Agent", "test"}, {"X-Method", "GET"}};
        enrich_opts_ssl(opts);
        opts.method = "GET";
    
        hc::http_resource src = session.open_url(URL, opts);
        // check
        std::string out{};
        out.resize(GET_RESPONSE.size());
        std::streamsize res = io::read_all(src, out);
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
    server.add_payload_handler("POST", "/", [](hs::http_request_ptr&) { return payload_receiver{}; });
    server.start();
    // client
    try {
        hc::http_session session{};
        hc::http_request_options opts{};
        opts.headers = {{"User-Agent", "test"}, {"X-Method", "POST"}};
        opts.method = "POST";
        enrich_opts_ssl(opts);
        io::string_source post_data{POSTPUT_DATA};
        hc::http_resource src = session.open_url(URL, post_data, opts);
        // check
        std::string out{};
        out.resize(POST_RESPONSE.size());        
        std::streamsize res = io::read_all(src, out);
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
    server.add_payload_handler("PUT", "/", [](hs::http_request_ptr&) {return payload_receiver{}; });
    server.start();
    // client
    try {
        hc::http_session session{};
        hc::http_request_options opts{};
        opts.headers = {{"User-Agent", "test"}, {"X-Method", "PUT"}};
        opts.method = "PUT";
        enrich_opts_ssl(opts);
        io::string_source post_data{POSTPUT_DATA};
        hc::http_resource src = session.open_url(URL, std::move(post_data), opts);
        // check
        std::string out{};
        out.resize(PUT_RESPONSE.size());
        std::streamsize res = io::read_all(src, out);
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
        hc::http_session session{};
        hc::http_request_options opts{};
        opts.headers = {{"User-Agent", "test"}, {"X-Method", "DELETE"}};
        enrich_opts_ssl(opts);
        opts.method = "DELETE";

        hc::http_resource src = session.open_url(URL, opts);
        // check
        std::string out{};
        out.resize(DELETE_RESPONSE.size());
        std::streamsize res = io::read_all(src, out);
        slassert(out.size() == static_cast<size_t> (res));
        slassert(DELETE_RESPONSE == out);
    } catch (const std::exception&) {
        server.stop(true);
        throw;
    }
    // stop server
    server.stop(true);
}

void test_connectfail() {    
    hc::http_session session{};
    hc::http_request_options opts{};
    opts.abort_on_connect_error = false;
    opts.connecttimeout_millis = 100;
    hc::http_resource src = session.open_url(URL, opts);
    std::array<char, 1> buf;
    auto res = src.read(buf);
    slassert(std::char_traits<char>::eof() == res);
    slassert(!src.connection_successful());
}

void test_stress() {
    hc::http_session session;
    auto fun = [&session] {
        hc::http_resource src = session.open_url("http://127.0.0.1:80/data");
        auto sink = cr::make_sha256_sink(io::null_sink());
        std::streamsize res = io::copy_all(src, sink);
        slassert(432515165 == res);
        slassert("1dfa11d08c8e721385b4a3717d575ec5a7bf85a7a7b7494e1aaa8583504c574b" == sink.get_hash());
        for (auto& ha : src.get_headers()) {
            std::cout << ha.first << ": " << ha.second << std::endl;
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 7; i++) {
        auto th = std::thread(fun);
        threads.emplace_back(std::move(th));
    }
    
    for (auto& th : threads) {
        th.join();
    }
    fun();
}

int main() {
    try {
        auto start = std::chrono::system_clock::now();
//        test_get();
//        test_post();
//        test_put();
//        test_delete();
//        test_connectfail();
        test_stress();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
        std::cout << "millist elapsed: " << elapsed.count() << std::endl;
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}

