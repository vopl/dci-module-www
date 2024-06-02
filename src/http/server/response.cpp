/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "response.hpp"
#include "request.hpp"
#include "../../enumSupport.hpp"

namespace dci::module::www::http::server
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Response::Response(Support* support, api::http::server::Response<>::Opposite&& api)
        : Base{support, std::move(api)}
    {
        // in firstLine(firstLine::Method, string path, firstLine::Version);
        _api.methods()->firstLine() += _sol * [this](api::http::firstLine::Version /*version*/, api::http::firstLine::StatusCode /*statusCode*/, primitives::String&& /*statusText*/)
        {
            dbgFatal("not impl");
            // bytes::Alter out{_buffer.end()};

            // std::optional<std::string_view> optStr = enumSupport::toString(version);
            // if(!optStr)
            // {
            //     _support->_failed.in(exception::buildInstance<api::http::error::UnknownRequestMethod>());
            //     _support->close();
            //     return;
            // }
            // out.write(optStr->data(), optStr->size());

            // out.write(" ");
            // std::string statusCodeStr = std::to_string(statusCode);
            // out.write(statusCodeStr.data(), statusCodeStr.size());
            // out.write(" ");
            // out.write(statusText.data(), statusText.size());
            // out.write("\r\n");
        };

        // in headers(list<Header>, bool done);
        _api.methods()->headers() += _sol * [this](const primitives::List<api::http::Header>& /*headers*/, bool /*done*/)
        {
            dbgFatal("not impl");
            // {
            //     bytes::Alter out{_buffer.end()};

            //     for(const api::http::Header& header : headers)
            //     {
            //         header.key.visit([&]<class K>(const K& value)
            //         {
            //             if constexpr(std::is_same_v<api::http::header::KeyRecognized, K>)
            //             {
            //                 std::optional<std::string_view> optStr = enumSupport::toString(value);
            //                 if(!optStr)
            //                 {
            //                     _support->_failed.in(exception::buildInstance<api::http::error::UnknownRequestVersion>());
            //                     _support->close();
            //                     return;
            //                 }
            //                 out.write(optStr->data(), optStr->size());
            //             }
            //             else
            //                 out.write(value.data(), value.size());
            //         });

            //         out.write(": ");
            //         out.write(header.value.data(), header.value.size());
            //         out.write("\r\n");
            //     }

            //     if(done)
            //         out.write("\r\n");
            // }

            // flushBuffer();
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
            apiDone();
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Response::~Response()
    {
        _sol.flush();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Response::requestFailed(inputSlicer::Result inputSlicerResult)
    {
        if(_someWote)
            return;

        switch(inputSlicerResult)
        {
        default:
        case inputSlicer::Result::badStatus:
            dbgAssert("bad inputSlicerResult value");
            [[fallthrough]];
        case inputSlicer::Result::badEntity:
            _buffer = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n";
            break;
        case inputSlicer::Result::badMethod:
            _buffer = "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n";
            break;
        case inputSlicer::Result::badVersion:
            _buffer = "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n";
            break;
        case inputSlicer::Result::tooBigContent:
            _buffer = "HTTP/1.1 413 Content Too Large\r\nConnection: close\r\n\r\n";
            break;
        case inputSlicer::Result::tooBigUri:
            _buffer = "HTTP/1.1 414 URI Too Long\r\nConnection: close\r\n\r\n";
            break;
        case inputSlicer::Result::tooBigHeaders:
            _buffer = "HTTP/1.1 431 Request Header Fields Too Large\r\nConnection: close\r\n\r\n";
            break;
        }

        flushBuffer();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Response::someWrote()
    {
        _someWote = true;
    }
}
