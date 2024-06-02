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
        inputSlicer::Result inputSlicerResult;
        {
            inputSlicer::SourceAdapter sa{data};
            inputSlicerResult = IS::process(sa);
        }

        ExceptionPtr err4Fail;
        switch(inputSlicerResult)
        {
        case inputSlicer::Result::needMore:
            dbgAssert(data.atBegin() && data.atEnd());
            if(hasDetacheableHeadersAccumuled())
                _api->headers(detachHeadersAccumuled(), false);
            if(hasDetacheableBodyAccumuled())
                _api->data(detachBodyAccumuled(), false);
            return io::InputProcessResult::needMore;

        case inputSlicer::Result::done:
            _response = nullptr;
            reset();
            if(_api)
            {
                _api->done();
                _api.reset();
            }
            return io::InputProcessResult::done;

        case inputSlicer::Result::badEntity:
            err4Fail = exception::buildInstance<api::http::error::request::BadRequest>();
            break;

        case inputSlicer::Result::badMethod:
            err4Fail = exception::buildInstance<api::http::error::request::BadMethod>();
            break;

        case inputSlicer::Result::badVersion:
            err4Fail = exception::buildInstance<api::http::error::request::BadVersion>();
            break;

        case inputSlicer::Result::tooBigContent:
            err4Fail = exception::buildInstance<api::http::error::request::TooBigContent>();
            break;

        case inputSlicer::Result::tooBigUri:
            err4Fail = exception::buildInstance<api::http::error::request::TooBigUri>();
            break;

        case inputSlicer::Result::tooBigHeaders:
            err4Fail = exception::buildInstance<api::http::error::request::TooBigHeaders>();
            break;

        default:
            unreacheable();
            break;
        }

        _response->requestFailed(inputSlicerResult);
        _support->failed(this, std::move(err4Fail));

        return io::InputProcessResult::bad;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceStart()
    {
        api::http::server::Request<> api;
        Base::setApi(api.init2());
        static_cast<Channel*>(_support)->emitIo(std::move(api));

        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceDone(inputSlicer::state::RequestFirstLine& firstLine)
    {
        //std::cout << "[" << firstLine._method <<"][" << firstLine._uri << "][" << firstLine._version << "]" << std::endl;

        inputSlicer::Result res = IS::sliceDone(firstLine);

        if(inputSlicer::Result::done == res)
            _api->firstLine(*firstLine._parsedMethod, std::move(firstLine._uri._downstream), *firstLine._parsedVersion);

        return res;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceDone(inputSlicer::state::Headers& headers)
    {
        // if(header._empty)
        //     std::cout << "[" << header._key <<"][" << header._value << "]" << std::endl;
        // else
        //     std::cout << "[" << header._key <<"][" << header._value << "]" << std::endl;

        inputSlicer::Result res = IS::sliceDone(headers);

        if(inputSlicer::Result::done == res)
            _api->headers(IS::detachHeadersAccumuled(), true);

        return res;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inputSlicer::Result Request::sliceDone(inputSlicer::state::Body& body)
    {
        // std::cout << "some body" << std::endl;

        inputSlicer::Result res = IS::sliceDone(body);

        if(inputSlicer::Result::done == res)
            _api->data(IS::detachBodyAccumuled(), true);

        return res;
    }

}
