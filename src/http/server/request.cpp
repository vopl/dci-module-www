/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "request.hpp"
#include "response.hpp"
#include "channel.hpp"
#include "../../enumSupport.hpp"

namespace dci::module::www::http::server
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Request::Request()
        : Base{}
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Request::~Request()
    {
        _sol.flush();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    bool /*done*/ Request::onReceived(bytes::Alter& data)
    {
        inputSlicer::SourceAdapter sa{data};

        switch(this->process(sa))
        {
        case inputSlicer::Result::needMoreInput:
            dbgAssert(data.atBegin() && data.atEnd());
            return false;
        case inputSlicer::Result::malformedInput:
            _support->close(exception::buildInstance<api::http::error::MalformedInputReceived>());
            break;
        case inputSlicer::Result::done:
            break;
        }

        return true;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceDone(const inputSlicer::state::RequestFirstLine& firstLine)
    {
        //std::cout << "[" << firstLine._method <<"][" << firstLine._uri << "][" << firstLine._version << "]" << std::endl;

        _sol.flush();
        if(_api)
        {
            _api->done();
            _api.reset();
        }

        std::optional<api::http::firstLine::Method> method = enumSupport::toEnum<api::http::firstLine::Method>(firstLine._method.str());
        if(!method)
            return inputSlicer::Result::malformedInput;

        std::optional<api::http::firstLine::Version> version = enumSupport::toEnum<api::http::firstLine::Version>(firstLine._version.str());
        if(!version)
            return inputSlicer::Result::malformedInput;

        {
            api::http::server::Request<> api;
            Base::setApi(api.init2());
            static_cast<Channel*>(_support)->emitIo(std::move(api));
        }

        _api->firstLine(*method, std::move(firstLine._uri._downstream), *version);
        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceDone(const inputSlicer::state::Header& header)
    {
        if(header._empty)
            std::cout << "[" << header._key <<"][" << header._value << "]" << std::endl;
        else
            std::cout << "[" << header._key <<"][" << header._value << "]" << std::endl;
        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceDone(const inputSlicer::state::Body& /*body*/)
    {
        std::cout << "some body" << std::endl;
        return inputSlicer::Result::done;
    }

}
