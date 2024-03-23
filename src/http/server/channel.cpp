/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "channel.hpp"

namespace dci::module::www::http::server
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Channel::Channel(idl::net::stream::Channel<> netStreamChannel)
        : api::http::server::Channel<>::Opposite{idl::interface::Initializer{}}
        , _ioPlexus{std::move(netStreamChannel)}
    {
        // in close();
        methods()->close() += sol() * [&]()
        {
            _ioPlexus.close();
        };

        // out closed();
        _ioPlexus._closed.out() += sol() * [&]()
        {
            methods()->closed();
        };

        // out failed(exception);
        _ioPlexus._failed.out() += sol() * [&](primitives::ExceptionPtr err)
        {
            methods()->failed(std::move(err));
        };

        // out upgradeHttp2(www::Channel::Opposite http2ServerChannel) -> bool;
        // out upgradeWs(www::Channel::Opposite wsChannel) -> bool;
        // out io(Request::Opposite, Response::Opposite);

        //_ioPlexus.emplace(std::tuple{}, std::tuple{});
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Channel::~Channel()
    {
        sol().flush();
    }
}
