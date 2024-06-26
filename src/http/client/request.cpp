/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "request.hpp"
#include "response.hpp"
#include "../../enumSupport.hpp"

namespace dci::module::www::http::client
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Request::Request(Support* support, api::http::client::Request<>::Opposite&& api)
        : Base{support, std::move(api)}
    {
        // in firstLine(firstLine::Method, string path, firstLine::Version);
        _api.methods()->firstLine() += _sol * [this](api::http::firstLine::Method method, primitives::String&& path, api::http::firstLine::Version version)
        {
            bytes::Alter out{_buffer.end()};

            std::optional<std::string_view> optStr = enumSupport::toString(method);
            if(!optStr)
            {
                _support->close(exception::buildInstance<api::http::error::request::BadMethod>());
                return;
            }
            out.write(optStr->data(), optStr->size());

            out.write(" ");
            out.write(path.data(), path.size());
            out.write(" ");

            optStr = enumSupport::toString(version);
            if(!optStr)
            {
                _support->close(exception::buildInstance<api::http::error::request::BadVersion>());
                return;
            }
            out.write(optStr->data(), optStr->size());
            out.write("\r\n");
        };

        // in headers(list<Header>, bool done);
        _api.methods()->headers() += _sol * [this](const primitives::List<api::http::Header>& headers, bool done)
        {
            {
                bytes::Alter out{_buffer.end()};

                for(const api::http::Header& header : headers)
                {
                    header.key.visit([&]<class K>(const K& value)
                    {
                        if constexpr(std::is_same_v<api::http::header::KeyRecognized, K>)
                        {
                            std::optional<std::string_view> optStr = enumSupport::toString(value);
                            if(!optStr)
                            {
                                _support->close(exception::buildInstance<api::http::error::request::BadRequest>());
                                return;
                            }
                            out.write(optStr->data(), optStr->size());
                        }
                        else
                            out.write(value.data(), value.size());
                    });

                    out.write(": ");
                    out.write(header.value.data(), header.value.size());
                    out.write("\r\n");
                }

                if(done)
                    out.write("\r\n");
            }

            flushBuffer();
        };

        // in data(bytes, bool done);
        _api.methods()->data() += _sol * [this](Bytes data, bool /*done*/)
        {
            _buffer.end().write(std::move(data));
            flushBuffer();
        };

        // in done();
        _api.methods()->done() += _sol * [this]()
        {
            flushBuffer();
            this->apiDone();
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Request::~Request()
    {
        _sol.flush();
    }
}
