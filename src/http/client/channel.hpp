/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "io/plexus.hpp"
#include "response.hpp"
#include "request.hpp"

namespace dci::module::www::http::client
{
    class Channel
        : public api::http::client::Channel<>::Opposite
        , public mm::heap::Allocable<Channel>
        , public io::Plexus<Response, Request, false>
    {
    public:
        Channel(idl::net::stream::Channel<> netStreamChannel);
        ~Channel();
    };
}
