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

namespace dci::module::www::http::client
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    class Request;

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    class Response
        : public io::InputBase<io::Plexus<Response, Request, false>, Response, false>
    {
        using Base = io::InputBase<io::Plexus<Response, Request, false>, Response, false>;
    public:
        Response(Support* support, api::http::client::Response<>::Opposite api);
        ~Response();

        bool /*done*/ onReceived(bytes::Alter data);
        void onFailed(primitives::ExceptionPtr);
        void onClosed();

    private:
        api::http::client::Response<>::Opposite _api;
    };
}
