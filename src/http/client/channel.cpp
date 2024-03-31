/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "channel.hpp"

namespace dci::module::www::http::client
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Channel::Channel(idl::net::stream::Channel<> netStreamChannel)
        : api::http::client::Channel<>::Opposite{idl::interface::Initializer{}}
        , io::Plexus<Response, Request, false>{std::move(netStreamChannel)}
    {
        // in close();
        methods()->close() += _sol * [&]()
        {
            io::Plexus<Response, Request, false>::close();
        };

        // out closed();
        io::Plexus<Response, Request, false>::_closed.out() += _sol * [&]()
        {
            methods()->closed();
        };

        // out failed(exception);
        io::Plexus<Response, Request, false>::_failed.out() += _sol * [&](primitives::ExceptionPtr err)
        {
            methods()->failed(std::move(err));
        };

        // in upgradeHttp2(www::Channel::Opposite http2ClientChannel) -> bool;
        methods()->upgradeHttp2() += _sol * [](api::Channel<>::Opposite /*http2ClientChannel*/)
        {
            dbgFatal("not impl");
            return cmt::readyFuture<bool>(exception::buildInstance<idl::interface::exception::MethodNotImplemented>());
        };

        // in upgradeWs(www::Channel::Opposite wsChannel) -> bool;
        methods()->upgradeWs() += _sol * [](api::Channel<>::Opposite /*wsChannel*/)
        {
            dbgFatal("not impl");
            return cmt::readyFuture<bool>(exception::buildInstance<idl::interface::exception::MethodNotImplemented>());
        };

        // in io(Request::Opposite, Response::Opposite);
        methods()->io() += _sol * [&](api::http::client::Request<>::Opposite request, api::http::client::Response<>::Opposite response)
        {
            io::Plexus<Response, Request, false>::emplace(std::tuple{std::move(response)}, std::tuple{std::move(request)});
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Channel::~Channel()
    {
        _sol.flush();
    }
}
