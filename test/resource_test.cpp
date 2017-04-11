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
 * File:   resource_test.cpp
 * Author: alex
 *
 * Created on November 20, 2015, 8:44 AM
 */

#include "staticlib/http/resource.hpp"

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

#include "staticlib/http/multi_threaded_session.hpp"
#include "staticlib/http/single_threaded_session.hpp"

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

bool throws_exc(std::function<void()> fun) {
    try {
        fun();
    } catch (const sl::http::http_exception& e) {
        (void) e;
        return true;
    }
    return false;
}

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

void get_handler(sl::httpserver::http_request_ptr& req, sl::httpserver::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("GET" == req->get_header("X-Method"));
    auto writer = sl::httpserver::http_response_writer::create(conn, req);
    writer->write(GET_RESPONSE);
    writer->send();
}

class response_sender : public std::enable_shared_from_this<response_sender> {
    std::mutex mutex;
    sl::httpserver::http_response_writer_ptr writer;
    std::unique_ptr<std::streambuf> stream;
    std::array<char, 4096> buf;

public:
    response_sender(sl::httpserver::http_response_writer_ptr writer,
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
            auto src = sl::io::streambuf_source(stream.get());
            size_t read = sl::io::read_all(src, buf);
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
            writer->get_connection()->set_lifecycle(sl::httpserver::tcp_connection::LIFECYCLE_CLOSE);
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

void post_handler(sl::httpserver::http_request_ptr& req, sl::httpserver::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("POST" == req->get_header("X-Method"));
    auto ph = req->get_payload_handler<payload_receiver>();
    slassert(nullptr != ph);
    slassert(ph->is_received());
    auto writer = sl::httpserver::http_response_writer::create(conn, req);
    writer->write(POST_RESPONSE);
    writer->send();
}

void put_handler(sl::httpserver::http_request_ptr& req, sl::httpserver::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("PUT" == req->get_header("X-Method"));
    auto ph = req->get_payload_handler<payload_receiver>();
    slassert(nullptr != ph);
    slassert(ph->is_received());
    auto writer = sl::httpserver::http_response_writer::create(conn, req);
    writer->write(PUT_RESPONSE);
    writer->send();
}

void delete_handler(sl::httpserver::http_request_ptr& req, sl::httpserver::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("DELETE" == req->get_header("X-Method"));
    auto writer = sl::httpserver::http_response_writer::create(conn, req);
    writer->write(DELETE_RESPONSE);
    writer->send();
}

void large_handler(sl::httpserver::http_request_ptr& req, sl::httpserver::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    auto writer = sl::httpserver::http_response_writer::create(conn, req);
    auto src = sl::io::make_unbuffered_istreambuf_ptr(sl::tinydir::file_source("/home/alex/vbox/hd/Android-x86_4.4_r4.7z"));
    auto sender = std::make_shared<response_sender>(writer, std::move(src));
    sender->send();
}

void timeout_handler(sl::httpserver::http_request_ptr& req, sl::httpserver::tcp_connection_ptr& conn) {
    slassert("test" == req->get_header("User-Agent"));
    std::this_thread::sleep_for(std::chrono::seconds(2));
    auto writer = sl::httpserver::http_response_writer::create(conn, req);
    writer->write(GET_RESPONSE);
    writer->send();
}

void request_get(sl::http::session& session) {
    sl::http::request_options opts{};
    opts.headers = {{"User-Agent", "test"}, {"X-Method", "GET"}};
    enrich_opts_ssl(opts);
    opts.method = "GET";
    sl::http::resource src = session.open_url(URL + "get", opts);
    // check
    std::string out{};
    out.resize(GET_RESPONSE.size());
    std::streamsize res = sl::io::read_all(src, out);
    slassert(out.size() == static_cast<size_t>(res));
    slassert(GET_RESPONSE == out);
}

void request_post(sl::http::session& session) {
    sl::http::request_options opts{};
    opts.headers = {{"User-Agent", "test"}, {"X-Method", "POST"}};
    opts.method = "POST";
    enrich_opts_ssl(opts);
    sl::io::string_source post_data{POSTPUT_DATA};
    sl::http::resource src = session.open_url(URL + "post", post_data, opts);
    // check
    std::string out{};
    out.resize(POST_RESPONSE.size());        
    std::streamsize res = sl::io::read_all(src, out);
    slassert(out.size() == static_cast<size_t> (res));
    slassert(POST_RESPONSE == out);
}

void request_put(sl::http::session& session) {
    sl::http::request_options opts{};
    opts.headers = {{"User-Agent", "test"}, {"X-Method", "PUT"}};
    opts.method = "PUT";
    enrich_opts_ssl(opts);
    sl::io::string_source post_data{POSTPUT_DATA};
    sl::http::resource src = session.open_url(URL + "put", std::move(post_data), opts);
    // check
    std::string out{};
    out.resize(PUT_RESPONSE.size());
    std::streamsize res = sl::io::read_all(src, out);
    slassert(out.size() == static_cast<size_t> (res));
    slassert(PUT_RESPONSE == out);
}

void request_delete(sl::http::session& session) {
    sl::http::request_options opts{};
    opts.headers = {{"User-Agent", "test"}, {"X-Method", "DELETE"}};
    enrich_opts_ssl(opts);
    opts.method = "DELETE";
    sl::http::resource src = session.open_url(URL + "delete", opts);
    // check
    std::string out{};
    out.resize(DELETE_RESPONSE.size());
    std::streamsize res = sl::io::read_all(src, out);
    slassert(out.size() == static_cast<size_t> (res));
    slassert(DELETE_RESPONSE == out);
}

void request_connectfail(sl::http::session& session) {    
    sl::http::request_options opts{};
    opts.abort_on_connect_error = false;
    opts.connecttimeout_millis = 100;
    sl::http::resource src = session.open_url(URL, opts);
    std::array<char, 1> buf;
    auto res = src.read(buf);
    slassert(std::char_traits<char>::eof() == res);
    slassert(!src.connection_successful());
}

void request_timeout(sl::http::session& session) {
    sl::http::request_options opts{};
    opts.headers = {{"User-Agent", "test"}};
    opts.timeout_millis = 3000;
    enrich_opts_ssl(opts);

    {
        sl::http::resource src = session.open_url(URL, opts);
        auto sink = sl::io::string_sink();
        std::streamsize res = sl::io::copy_all(src, sink);
        slassert(GET_RESPONSE.size() == static_cast<size_t> (res));
        slassert(GET_RESPONSE == sink.get_string());
    }
    
    {
        bool caught = false;
        // timeout
        opts.timeout_millis = 1000;
        try {
            sl::http::resource src = session.open_url(URL, opts);
            auto sink = sl::io::null_sink();
            sl::io::copy_all(src, sink);
        } catch(const sl::http::http_exception& e) {
            (void) e;
            caught = true;
        }
        slassert(caught);
    }
}

void request_status(sl::http::session& session) {
    auto opts = sl::http::request_options();
    opts.headers = {{"User-Agent", "test"}, {"X-Method", "GET"}};
    enrich_opts_ssl(opts);
    opts.method = "GET";
    opts.abort_on_response_error = false;
    // perform
    {
        auto res_success = session.open_url(URL + "get", opts);
        slassert(200 == res_success.get_status_code());
    }
    {
        auto res_fail = session.open_url(URL + "get_FAIL", opts);
        slassert(404 == res_fail.get_status_code());
    }
}

void test_stress() {
    auto session = sl::http::multi_threaded_session();
    
    auto fun = [&session] {
        sl::http::request_options opts;
        opts.timeout_millis = 60000;
        sl::http::resource src = session.open_url("http://127.0.0.1:80/data", opts);
        auto sink = sl::crypto::make_sha256_sink(sl::io::null_sink());
        std::streamsize res = sl::io::copy_all(src, sink);
        slassert(432515165 == res);
        slassert("1dfa11d08c8e721385b4a3717d575ec5a7bf85a7a7b7494e1aaa8583504c574b" == sink.get_hash());
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

void test_methods() {
    // server
    sl::httpserver::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("GET", "/get", get_handler);
    server.add_handler("POST", "/post", post_handler);
    server.add_payload_handler("POST", "/post", [](sl::httpserver::http_request_ptr&) { return payload_receiver{}; });
    server.add_handler("PUT", "/put", put_handler);
    server.add_payload_handler("PUT", "/put", [](sl::httpserver::http_request_ptr&) { return payload_receiver{}; });    
    server.add_handler("DELETE", "/delete", delete_handler);
    server.start();
    try {
        auto st = sl::http::single_threaded_session();
        auto mt = sl::http::multi_threaded_session();
        request_get(st);
        request_get(mt);
        request_post(st);
        request_post(mt);
        request_put(st);
        request_put(mt);
        request_delete(st);
        request_delete(mt);
        request_status(st);
        request_status(mt);
    } catch (const std::exception&) {
        server.stop(true);
        throw;
    }
    // stop server
    server.stop(true);
}

void test_connectfail() {
    auto st = sl::http::single_threaded_session();
    auto mt = sl::http::multi_threaded_session();
    request_connectfail(st);
    request_connectfail(mt);
}

void test_single() {
    auto st = sl::http::single_threaded_session();
    auto opts = sl::http::request_options();
    opts.abort_on_connect_error = false;
    opts.connecttimeout_millis = 100;
    {
        auto res1 = st.open_url(URL, opts);
        slassert(throws_exc([&]{
            auto res2 = st.open_url(URL, opts);
        }));
    }
    auto res3 = st.open_url(URL, opts);
}

void test_timeout() {
    // server
    sl::httpserver::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("GET", "/", timeout_handler);
    server.start();
    try {
        auto st = sl::http::single_threaded_session();
        auto mt = sl::http::multi_threaded_session();
        request_timeout(st);
        request_timeout(mt);
    } catch (const std::exception&) {
        server.stop(true);
        throw;
    }
    // stop server
    server.stop(true);
}

void test_status_fail() {
    auto st = sl::http::single_threaded_session();    
    auto opts = sl::http::request_options();
    opts.connecttimeout_millis = 100;
    slassert(throws_exc([&] {
        st.open_url(URL + "get", opts);
    }));
    auto mt = sl::http::multi_threaded_session();
    slassert(throws_exc([&] {
        mt.open_url(URL + "get", opts);
    }));
}

int main() {
    try {
//        auto start = std::chrono::system_clock::now();
        test_methods();
        test_connectfail();
        test_single();
//        test_timeout();
        test_status_fail();
//        test_stress();
//        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
//        std::cout << "millist elapsed: " << elapsed.count() << std::endl;
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}

