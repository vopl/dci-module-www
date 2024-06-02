/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "io/plexus.hpp"
#include "io/inputBase.hpp"
#include "../inputSlicer.hpp"

namespace dci::module::www::http::server
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    class Response;

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    class Request
        : public io::InputBase<io::Plexus<Request, Response, true>, Request, api::http::server::Request<>::Opposite, true>
        , protected InputSlicer<inputSlicer::Mode::request, Request>
    {
        using Support = io::Plexus<Request, Response, true>;
        using Base = io::InputBase<io::Plexus<Request, Response, true>, Request, api::http::server::Request<>::Opposite, true>;
        using IS = InputSlicer<inputSlicer::Mode::request, Request>;

    public:
        Request();
        ~Request();

        void setResponse(Response* response);
        io::InputProcessResult process(bytes::Alter& data);

    private:
        friend IS;
        inputSlicer::Result sliceStart();
        inputSlicer::Result sliceDone(inputSlicer::state::RequestFirstLine& firstLine);
        inputSlicer::Result sliceDone(inputSlicer::state::Headers& headers);
        inputSlicer::Result sliceDone(inputSlicer::state::Body& body);

    private:
        Response* _response{};
    };
}
