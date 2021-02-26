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
 * File:   polling_resource.hpp
 * Author: alex
 *
 * Created on March 25, 2017, 9:10 PM
 */

#ifndef STATICLIB_HTTP_POLLING_RESOURCE_HPP
#define STATICLIB_HTTP_POLLING_RESOURCE_HPP

#include "staticlib/http/resource.hpp"

namespace staticlib {
namespace http {

// forward decl
class resource_params;

class polling_resource : public resource {
protected:
    class impl;

public:
    PIMPL_INHERIT_CONSTRUCTOR(polling_resource, resource)

    polling_resource(uint64_t resource_id, const std::string& url);

    polling_resource(uint64_t resource_id, const std::string& url, resource_info&& info, uint16_t status_code,
            std::vector<std::pair<std::string, std::string>>&& response_headers,
            std::vector<char>&& data, const std::string& data_file_path);

    virtual std::streamsize read(sl::io::span<char> span) override;

    virtual const std::string& get_url() const override;

    virtual uint16_t get_status_code() const override;

    virtual resource_info get_info() const override;

    virtual const std::vector<std::pair<std::string, std::string>>& get_headers() const override;

    virtual const std::string& get_header(const std::string& name) const override;

    virtual bool connection_successful() const override;

    virtual uint64_t get_id() const override;

    virtual const std::string& get_response_data_file() const override;

};

} // namespace
}

#endif /* STATICLIB_HTTP_POLLING_RESOURCE_HPP */
