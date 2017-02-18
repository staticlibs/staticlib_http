/* 
 * File:   running_request.hpp
 * Author: alex
 *
 * Created on February 13, 2017, 9:23 PM
 */

#ifndef STATICLIB_HTTPCLIENT_RUNNING_REQUEST_HPP
#define	STATICLIB_HTTPCLIENT_RUNNING_REQUEST_HPP

#include <cstdint>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "curl/curl.h"

#include "staticlib/config.hpp"

#include "staticlib/httpclient/http_request_options.hpp"
#include "staticlib/httpclient/http_resource_info.hpp"

#include "curl_deleters.hpp"
#include "curl_headers.hpp"
#include "running_request_pipe.hpp"
#include "request_ticket.hpp"

namespace staticlib {
namespace httpclient {

// todo: naming

class running_request {
    enum class req_state {
        created, receiving_headers, receiving_data
    };
    std::chrono::system_clock::time_point started_time_point;
    
    // holds data passed to curl
    std::string url;
    http_request_options options;
    std::unique_ptr<std::istream> post_data;
    curl_headers headers;
    std::unique_ptr<CURL, curl_easy_deleter> handle;

    std::shared_ptr<running_request_pipe> pipe;

    bool paused = false;
    std::string error;
    
    req_state state = req_state::created;
    
public:
    running_request(CURLM* multi_handle, request_ticket&& ticket) :
    started_time_point(std::chrono::system_clock::now()),
    url(std::move(ticket.url)),
    options(std::move(ticket.options)),
    post_data(std::move(ticket.post_data)),    
    handle(curl_easy_init(), curl_easy_deleter(multi_handle)),
    pipe(std::move(ticket.pipe)) {        
        if (nullptr == handle.get()) throw httpclient_exception(TRACEMSG("Error initializing cURL handle"));
        CURLMcode errm = curl_multi_add_handle(multi_handle, handle.get());
        if (errm != CURLM_OK) throw httpclient_exception(TRACEMSG(
                "cURL multi_add error: [" + curl_multi_strerror(errm) + "], url: [" + this->url + "]"));
        apply_options();
    }
    
    running_request(const running_request&) = delete;
    
    running_request& operator=(const running_request&) = delete;
    
    // finalization must be noexcept anyway,
    // so lets tie finalization to destruction
    ~running_request() STATICLIB_NOEXCEPT {
        auto info = [this] {
            try {
                return collect_info();
            } catch (const std::exception& e) {
                append_error(TRACEMSG(e.what()));
                return http_resource_info();
            }
        }();
        pipe->set_resource_info(std::move(info));
        if (!error.empty()) {
            pipe->append_error(error);
        }
        pipe->finalize_data_queue();
    }

    static size_t headers_callback(char* buffer, size_t size, size_t nitems, void* userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t> (-1);
        running_request* ptr = static_cast<running_request*> (userp);
        try {
            return ptr->write_headers(buffer, size, nitems);
        } catch (const std::exception& e) {
            ptr->append_error(TRACEMSG(e.what()));
            return 0;
        }
    }

    static size_t write_callback(char* buffer, size_t size, size_t nitems, void* userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t> (-1);
        running_request* ptr = static_cast<running_request*> (userp);
        try {
            return ptr->write_data(buffer, size, nitems);
        } catch (const std::exception& e) {
            ptr->append_error(TRACEMSG(e.what()));
            return 0;
        }
    }

    static size_t read_callback(char *buffer, size_t size, size_t nitems, void *userp) STATICLIB_NOEXCEPT {
        if (nullptr == userp) return static_cast<size_t> (-1);
        running_request* ptr = static_cast<running_request*> (userp);
        try {
            return ptr->read_data(buffer, size, nitems);
        } catch (const std::exception& e) {
            ptr->append_error(TRACEMSG(e.what()));
            return CURL_READFUNC_ABORT;
        }
    }
    
    const std::string& get_url() const {
        return url;
    }
    
    bool is_paused() {
        return paused;
    }
    
    void unpause() {
        this->paused = false;
        curl_easy_pause(handle.get(), CURLPAUSE_CONT);
    }

    void append_error(const std::string& msg) {
        if (error.empty()) {
            error.append("Error reported for request, url: [" + url + "]\n");
        } else {
            error.append("\n");
        }
        error.append(msg);
    }
    
    http_request_options& get_options() {
        return options;
    }
    
    CURL* easy_handle() {
        return handle.get();
    }
    
    std::chrono::system_clock::time_point started_at() {
        return started_time_point;
    }
    
    bool data_queue_is_full() {
        return pipe->data_queue_is_full();
    }
    
private:
    // http://stackoverflow.com/a/9681122/314015
    size_t write_headers(char *buffer, size_t size, size_t nitems) {
        namespace sc = staticlib::config;
        if (req_state::created == state) {
            long code = getinfo_long(CURLINFO_RESPONSE_CODE);
            if (options.abort_on_response_error && code >= 400) {
                append_error("HTTP response error, status code: [" + sc::to_string(code) + "]");
                return 0;
            } else {
                pipe->set_response_code(code);
                state = req_state::receiving_headers;
            }
        }
        size_t len = size*nitems;
        std::string name{};
        for (size_t i = 0; i < len; i++) {
            if (':' != buffer[i]) {
                name.push_back(buffer[i]);
            } else {
                std::string value{};
                // 2 for ': ', 2 for '\r\n'
                size_t valen = len - i - 2 - 2;
                if (valen > 0) {
                    value.resize(valen);
                    std::memcpy(std::addressof(value.front()), buffer + i + 2, value.length());
                    pipe->emplace_header(std::move(name), std::move(value));                        
                    break;
                }
            }
        }
        return len;
    }
    
    size_t write_data(char* buffer, size_t size, size_t nitems) {
        size_t len = size * nitems;
        // chunk is too big for stack        
        std::vector<char> buf;
        buf.resize(len);
        std::memcpy(buf.data(), buffer, buf.size());
        bool placed = pipe->emplace_some_data(std::move(buf));
        if (!placed) {
            paused = true;
            return CURL_WRITEFUNC_PAUSE;
        }
        return len;
    }

    size_t read_data(char* buffer, size_t size, size_t nitems) {
        size_t len = size * nitems;
        auto src = io::streambuf_source(post_data->rdbuf());
        std::streamsize read = io::read_all(src, {buffer, len});
        return static_cast<size_t> (read);
    }

    void apply_options() {
        // url
        setopt_string(CURLOPT_URL, url);

        // method
        appply_method();

        // headers
        auto slist = headers.wrap_into_slist(this->options.headers);
        if (slist.has_value()) {
            setopt_object(CURLOPT_HTTPHEADER, static_cast<void*> (slist.value()));
        }

        // callbacks
        setopt_object(CURLOPT_WRITEDATA, this);
        CURLcode err_wf = curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, running_request::write_callback);
        if (err_wf != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [CURLOPT_WRITEFUNCTION], error: [" + curl_easy_strerror(err_wf) + "]"));
        setopt_object(CURLOPT_HEADERDATA, this);
        CURLcode err_hf = curl_easy_setopt(handle.get(), CURLOPT_HEADERFUNCTION, running_request::headers_callback);
        if (err_hf != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [CURLOPT_HEADERFUNCTION], error: [" + curl_easy_strerror(err_hf) + "]"));

        // general behavior options
        if (options.force_http_10) {
            CURLcode err = curl_easy_setopt(handle.get(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
            if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                    "Error setting option: [CURLOPT_HTTP_VERSION], error: [" + curl_easy_strerror(err) + "]"));
        }
        setopt_bool(CURLOPT_NOPROGRESS, options.noprogress);
        setopt_bool(CURLOPT_NOSIGNAL, options.nosignal);
        setopt_bool(CURLOPT_FAILONERROR, options.failonerror);
        // Aded in 7.42.0 
#if LIBCURL_VERSION_NUM >= 0x072a00
        setopt_bool(CURLOPT_PATH_AS_IS, options.path_as_is);
#endif // LIBCURL_VERSION_NUM

        // TCP options
        setopt_bool(CURLOPT_TCP_NODELAY, options.tcp_nodelay);
        setopt_bool(CURLOPT_TCP_KEEPALIVE, options.tcp_keepalive);
        setopt_uint32(CURLOPT_TCP_KEEPIDLE, options.tcp_keepidle_secs);
        setopt_uint32(CURLOPT_TCP_KEEPINTVL, options.tcp_keepintvl_secs);
        setopt_uint32(CURLOPT_CONNECTTIMEOUT_MS, options.connecttimeout_millis);
        setopt_uint32(CURLOPT_TIMEOUT_MS, options.timeout_millis);

        // HTTP options
        setopt_uint32(CURLOPT_BUFFERSIZE, options.buffersize_bytes);
        setopt_string(CURLOPT_ACCEPT_ENCODING, options.accept_encoding);
        setopt_bool(CURLOPT_FOLLOWLOCATION, options.followlocation);
        setopt_uint32(CURLOPT_MAXREDIRS, options.maxredirs);
        setopt_string(CURLOPT_USERAGENT, options.useragent);

        // throttling options
        setopt_uint32(CURLOPT_MAX_SEND_SPEED_LARGE, options.max_sent_speed_large_bytes_per_second);
        setopt_uint32(CURLOPT_MAX_RECV_SPEED_LARGE, options.max_recv_speed_large_bytes_per_second);

        // SSL options
        setopt_string(CURLOPT_SSLCERT, options.sslcert_filename);
        setopt_string(CURLOPT_SSLCERTTYPE, options.sslcertype);
        setopt_string(CURLOPT_SSLKEY, options.sslkey_filename);
        setopt_string(CURLOPT_SSLKEYTYPE, options.ssl_key_type);
        setopt_string(CURLOPT_KEYPASSWD, options.ssl_keypasswd);
        if (options.require_tls) {
            CURLcode err = curl_easy_setopt(handle.get(), CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
            if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                    "Error setting option: [CURLOPT_SSLVERSION], error: [" + curl_easy_strerror(err) + "]"));
        }
        if (options.ssl_verifyhost) {
            setopt_uint32(CURLOPT_SSL_VERIFYHOST, 2);
        } else {
            setopt_bool(CURLOPT_SSL_VERIFYHOST, false);
        }
        setopt_bool(CURLOPT_SSL_VERIFYPEER, options.ssl_verifypeer);
        // Added in 7.41.0
#if LIBCURL_VERSION_NUM >= 0x072900        
        setopt_bool(CURLOPT_SSL_VERIFYSTATUS, options.ssl_verifystatus);
#endif // LIBCURL_VERSION_NUM        
        setopt_string(CURLOPT_CAINFO, options.cainfo_filename);
        setopt_string(CURLOPT_CRLFILE, options.crlfile_filename);
        setopt_string(CURLOPT_SSL_CIPHER_LIST, options.ssl_cipher_list);
    }

    long getinfo_long(CURLINFO opt) {
        namespace sc = staticlib::config;
        long out = -1;
        CURLcode err = curl_easy_getinfo(handle.get(), opt, std::addressof(out));
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "cURL curl_easy_getinfo error: [" + curl_easy_strerror(err) + "]," +
                " option: [" + sc::to_string(opt) + "]"));
        return out;
    }

    double getinfo_double(CURLINFO opt) {
        namespace sc = staticlib::config;
        double out = -1;
        CURLcode err = curl_easy_getinfo(handle.get(), opt, std::addressof(out));
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "cURL curl_easy_getinfo error: [" + curl_easy_strerror(err) + "]," +
                " option: [" + sc::to_string(opt) + "]"));
        return out;
    }

    std::string getinfo_string(CURLINFO opt) {
        namespace sc = staticlib::config;
        char* out = nullptr;
        CURLcode err = curl_easy_getinfo(handle.get(), opt, std::addressof(out));
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "cURL curl_easy_getinfo error: [" + curl_easy_strerror(err) + "]," +
                " option: [" + sc::to_string(opt) + "]"));
        return std::string(out);
    }

    http_resource_info collect_info() {
        http_resource_info info;
        info.effective_url = getinfo_string(CURLINFO_EFFECTIVE_URL);
        info.total_time_secs = getinfo_double(CURLINFO_TOTAL_TIME);
        info.namelookup_time_secs = getinfo_double(CURLINFO_NAMELOOKUP_TIME);
        info.connect_time_secs = getinfo_double(CURLINFO_CONNECT_TIME);
        info.appconnect_time_secs = getinfo_double(CURLINFO_APPCONNECT_TIME);
        info.pretransfer_time_secs = getinfo_double(CURLINFO_PRETRANSFER_TIME);
        info.starttransfer_time_secs = getinfo_double(CURLINFO_STARTTRANSFER_TIME);
        info.redirect_time_secs = getinfo_double(CURLINFO_REDIRECT_TIME);
        info.redirect_count = getinfo_long(CURLINFO_REDIRECT_COUNT);
        info.speed_download_bytes_secs = getinfo_double(CURLINFO_SPEED_DOWNLOAD);
        info.speed_upload_bytes_secs = getinfo_double(CURLINFO_SPEED_UPLOAD);
        info.header_size_bytes = getinfo_long(CURLINFO_HEADER_SIZE);
        info.request_size_bytes = getinfo_long(CURLINFO_REQUEST_SIZE);
        info.ssl_verifyresult = getinfo_long(CURLINFO_SSL_VERIFYRESULT);
        info.os_errno = getinfo_long(CURLINFO_OS_ERRNO);
        info.num_connects = getinfo_long(CURLINFO_NUM_CONNECTS);
        info.primary_ip = getinfo_string(CURLINFO_PRIMARY_IP);
        info.primary_port = getinfo_long(CURLINFO_PRIMARY_PORT);
        return info;
    }

    // note: think about integer overflow 

    void setopt_uint32(CURLoption opt, uint32_t value) {
        namespace sc = staticlib::config;
        if (0 == value) return;
        if (value > static_cast<uint32_t> (std::numeric_limits<int32_t>::max())) throw httpclient_exception(TRACEMSG(
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to invalid overflow value: [" + sc::to_string(value) + "]"));
        CURLcode err = curl_easy_setopt(handle.get(), opt, static_cast<long> (value));
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to value: [" + sc::to_string(value) + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    void setopt_bool(CURLoption opt, bool value) {
        namespace sc = staticlib::config;
        CURLcode err = curl_easy_setopt(handle.get(), opt, value ? 1 : 0);
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to value: [" + sc::to_string(value) + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    void setopt_string(CURLoption opt, const std::string& value) {
        namespace sc = staticlib::config;
        if ("" == value) return;
        CURLcode err = curl_easy_setopt(handle.get(), opt, value.c_str());
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " to value: [" + value + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    void setopt_object(CURLoption opt, void* value) {
        namespace sc = staticlib::config;
        if (nullptr == value) return;
        CURLcode err = curl_easy_setopt(handle.get(), opt, value);
        if (err != CURLE_OK) throw httpclient_exception(TRACEMSG(
                "Error setting option: [" + sc::to_string(opt) + "]," +
                " error: [" + curl_easy_strerror(err) + "]"));
    }

    struct curl_slist* append_header(struct curl_slist* slist, const std::string& header) {
        struct curl_slist* res = curl_slist_append(slist, header.c_str());
        if (nullptr == res) throw httpclient_exception(TRACEMSG(
                "Error appending header: [" + header + "]"));
        return res;
    }

    void appply_method() {
        if ("" == options.method) return;
        if ("GET" == options.method) {
            setopt_bool(CURLOPT_HTTPGET, true);
        } else if ("POST" == options.method) {
            setopt_bool(CURLOPT_POST, true);
        } else if ("PUT" == options.method) {
            setopt_bool(CURLOPT_PUT, true);
        } else if ("DELETE" == options.method) {
            setopt_string(CURLOPT_CUSTOMREQUEST, "DELETE");
        } else throw httpclient_exception(TRACEMSG(
                "Unsupported HTTP method: [" + options.method + "]"));
        if (nullptr != post_data.get() && ("POST" == options.method || "PUT" == options.method)) {
            setopt_object(CURLOPT_READDATA, this);
            CURLcode err_wf = curl_easy_setopt(handle.get(), CURLOPT_READFUNCTION, running_request::read_callback);
            if (err_wf != CURLE_OK) throw httpclient_exception(TRACEMSG(
                    "Error setting option: [CURLOPT_READFUNCTION], error: [" + curl_easy_strerror(err_wf) + "]"));
            options.headers.emplace_back("Transfer-Encoding", "chunked");
        }
    }   
};

} // namespace
}


#endif	/* STATICLIB_HTTPCLIENT_RUNNING_REQUEST_HPP */

