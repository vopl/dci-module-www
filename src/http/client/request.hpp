/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "io/plexus.hpp"
#include "io/outputBase.hpp"

namespace dci::module::www::http::client
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    class Response;

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    class Request
        : public io::OutputBase<io::Plexus<Response, Request>, Request>
    {
        using Base = io::OutputBase<io::Plexus<Response, Request>, Request>;
    public:
        Request(Support* support, api::http::client::Request<>::Opposite api);
        ~Request();

        void allowWrite();

    private:
        void flushBuffer();

    private:
        api::http::client::Request<>::Opposite  _api;
        bool                                    _writeAllowed{};
        Bytes                                   _buffer;
        sbs::Owner                              _sol;
    };
}
