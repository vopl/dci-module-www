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
        , _response{}
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Request::~Request()
    {
        _sol.flush();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Request::setResponse(Response* response)
    {
        _response = response;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    io::InputProcessResult Request::process(bytes::Alter& data)
    {
        inputSlicer::SourceAdapter sa{data};

        inputSlicer::Result inputSlicerResult = IS::process(sa);
        switch(inputSlicerResult)
        {
        case inputSlicer::Result::needMore:
            dbgAssert(data.atBegin() && data.atEnd());
            if(hasDetacheableHeadersAccumuled(true))
                _api->headers(detachHeadersAccumuled(true), false);
            return io::InputProcessResult::needMore;

        case inputSlicer::Result::done:
            _response = nullptr;
            if(_api)
            {
                _api->done();
                reset();
            }
            return io::InputProcessResult::done;

        default:
            break;
        }

         _response->requestFailed(inputSlicerResult);
        _response = nullptr;
        return io::InputProcessResult::bad;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceStart()
    {
        reset();

        api::http::server::Request<> api;
        Base::setApi(api.init2());
        static_cast<Channel*>(_support)->emitIo(std::move(api));

        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceDone(inputSlicer::state::RequestFirstLine& firstLine)
    {
        //std::cout << "[" << firstLine._method <<"][" << firstLine._uri << "][" << firstLine._version << "]" << std::endl;

        std::optional<api::http::firstLine::Method> method = enumSupport::toEnum<api::http::firstLine::Method>(firstLine._method.str());
        if(!method)
            return inputSlicer::Result::badMethod;

        std::optional<api::http::firstLine::Version> version = enumSupport::toEnum<api::http::firstLine::Version>(firstLine._version.str());
        if(!version)
            return inputSlicer::Result::badVersion;

        _api->firstLine(*method, std::move(firstLine._uri._downstream), *version);
        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceDone(inputSlicer::state::Header& header)
    {
        // if(header._empty)
        //     std::cout << "[" << header._key <<"][" << header._value << "]" << std::endl;
        // else
        //     std::cout << "[" << header._key <<"][" << header._value << "]" << std::endl;

        if(inputSlicer::state::Header::Kind::empty == header._kind)
        {
            _api->headers(IS::detachHeadersAccumuled(false), true);
            return inputSlicer::Result::done;
        }

        return IS::sliceDone(header);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceDone(inputSlicer::state::Body& /*body*/)
    {
        std::cout << "some body" << std::endl;
        //_api->data(xxx);
        return inputSlicer::Result::done;
    }

}
