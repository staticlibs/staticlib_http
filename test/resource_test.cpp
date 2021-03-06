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
#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include "asio.hpp"

#include "staticlib/pion.hpp"

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

void get_handler(sl::pion::http_request_ptr req, sl::pion::response_writer_ptr resp) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("GET" == req->get_header("X-Method"));
    resp->write(GET_RESPONSE);
    resp->send(std::move(resp));
}

void get_5_sec_handler(sl::pion::http_request_ptr req, sl::pion::response_writer_ptr resp) {
//    std::cout << ">>> server5: " << std::this_thread::get_id() << std::endl;
    (void) req;
    std::this_thread::sleep_for(std::chrono::seconds{5});
    resp->write("5 sec resp");
    resp->send(std::move(resp));
//    std::cout << "<<< server5: " << std::this_thread::get_id() << std::endl;
}

void get_1_sec_handler(sl::pion::http_request_ptr req, sl::pion::response_writer_ptr resp) {
//    std::cout << ">>> server1: " << std::this_thread::get_id() << std::endl;
    (void) req;
    std::this_thread::sleep_for(std::chrono::seconds{1});
    resp->write("1 sec resp");
    resp->send(std::move(resp));
//    std::cout << "<<< server1: " << std::this_thread::get_id() << std::endl;
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

void put_handler(sl::pion::http_request_ptr req, sl::pion::response_writer_ptr resp) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("PUT" == req->get_header("X-Method"));
    auto ph = req->get_payload_handler<payload_receiver>();
    slassert(nullptr != ph);
    slassert(ph->is_received());
    resp->write(PUT_RESPONSE);
    resp->send(std::move(resp));
}

void delete_handler(sl::pion::http_request_ptr req, sl::pion::response_writer_ptr resp) {
    slassert("test" == req->get_header("User-Agent"));
    slassert("DELETE" == req->get_header("X-Method"));
    resp->write(DELETE_RESPONSE);
    resp->send(std::move(resp));
}

void timeout_handler(sl::pion::http_request_ptr req, sl::pion::response_writer_ptr resp) {
    slassert("test" == req->get_header("User-Agent"));
    std::this_thread::sleep_for(std::chrono::seconds(2));
    resp->write(GET_RESPONSE);
    resp->send(std::move(resp));
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
        slassert(3537494016 == res);
        slassert("e401808d47bd814d35b74e1e321b4412b842be5b71cf9027d437a9456b677311" == sink.get_hash());
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
    sl::pion::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), 10000, SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("GET", "/get", get_handler);
    server.add_handler("POST", "/post", post_handler);
    server.add_payload_handler("POST", "/post", [](sl::pion::http_request_ptr&) { return payload_receiver{}; });
    server.add_handler("PUT", "/put", put_handler);
    server.add_payload_handler("PUT", "/put", [](sl::pion::http_request_ptr&) { return payload_receiver{}; });    
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
    sl::pion::http_server server(2, TCP_PORT, asio::ip::address_v4::any(), 10000, SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
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

void test_queue() {
    sl::pion::http_server server(4, TCP_PORT, asio::ip::address_v4::any(), 10000, SERVER_CERT_PATH, pwdcb, CA_PATH, verifier);
    server.add_handler("GET", "/get5", get_5_sec_handler);
    server.add_handler("GET", "/get1", get_1_sec_handler);
    server.start();
    try {
        auto mt = sl::http::multi_threaded_session();
        auto opts = sl::http::request_options();
        enrich_opts_ssl(opts);
        std::atomic<uint16_t> counter{0};
        auto vec = std::vector<std::thread>();
        vec.emplace_back([&mt, &opts, &counter] {
//            std::cout << ">>> client5: " << std::this_thread::get_id() << std::endl;
            auto src = mt.open_url(URL + "get5", opts);
            auto sink = sl::io::string_sink();
            sl::io::copy_all(src, sink);
            slassert(10 == counter.load(std::memory_order_acquire));
//            std::cout << "<<< client5: " << std::this_thread::get_id() << ": " << sink.get_string() << std::endl;
        });
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        for (int i = 0; i < 10; i++) {
            vec.emplace_back([&mt, &opts, &counter] {
//                std::cout << ">>> client1: " << std::this_thread::get_id() << std::endl;
                auto src = mt.open_url(URL + "get1", opts);
                auto sink = sl::io::string_sink();
                sl::io::copy_all(src, sink);
//                std::cout << "<<< client1: " << std::this_thread::get_id() << ": " << sink.get_string() << std::endl;
                counter.fetch_add(1, std::memory_order_acq_rel);
            });
        }
        for (auto&& th : vec) {
            th.join();
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
//        auto start = std::chrono::system_clock::now();
        test_methods();
        test_connectfail();
        test_single();
        test_status_fail();
//        test_stress();
//        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
//        std::cout << "millis elapsed: " << elapsed.count() << std::endl;
//        test_timeout();
//        test_queue();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}

