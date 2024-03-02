/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "factory.hpp"
#include "tls/client/channel.hpp"
#include "tls/server/channel.hpp"
#include "http/client/channel.hpp"
#include "http/server/channel.hpp"
#include "http2/client/channel.hpp"
#include "http2/server/channel.hpp"
#include "ws/channel.hpp"

namespace dci::module::www
{
    namespace
    {
        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <class Impl>
        auto createImpl(auto&&... args)
        {
            Impl* impl = new Impl{std::forward<decltype(args)>(args)...};
            impl->involvedChanged() += impl->sol() * [impl](bool v)
            {
                if(!v)
                    delete impl;
            };

            return impl->opposite();
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Factory::Factory(host::Manager* hostManager)
        : api::Factory<>::Opposite{idl::interface::Initializer{}}
        , _hostManager{hostManager}
    {
        // in tlsClientChannel(net::stream::Channel) -> tls::client::Channel;
        methods()->tlsClientChannel() += sol() * [](idl::net::stream::Channel<> netStreamChannel)
        {
            return cmt::readyFuture(createImpl<tls::client::Channel>(std::move(netStreamChannel)));
        };

        // in tlsServerChannel(net::stream::Channel) -> tls::server::Channel;
        methods()->tlsServerChannel() += sol() * [](idl::net::stream::Channel<> netStreamChannel)
        {
            return cmt::readyFuture(createImpl<tls::server::Channel>(std::move(netStreamChannel)));
        };

        // in httpClientChannel(net::stream::Channel) -> http::client::Channel;
        methods()->httpClientChannel() += sol() * [](idl::net::stream::Channel<> netStreamChannel)
        {
            return cmt::readyFuture(createImpl<http::client::Channel>(std::move(netStreamChannel)));
        };

        // in httpServerChannel(net::stream::Channel) -> http::server::Channel;
        methods()->httpServerChannel() += sol() * [](idl::net::stream::Channel<> netStreamChannel)
        {
            return cmt::readyFuture(createImpl<http::server::Channel>(std::move(netStreamChannel)));
        };

        // in http2ClientChannel(net::stream::Channel) -> http2::client::Channel;
        methods()->http2ClientChannel() += sol() * [](idl::net::stream::Channel<> netStreamChannel)
        {
            return cmt::readyFuture(createImpl<http2::client::Channel>(std::move(netStreamChannel)));
        };

        // in http2ServerChannel(net::stream::Channel) -> http2::server::Channel;
        methods()->http2ServerChannel() += sol() * [](idl::net::stream::Channel<> netStreamChannel)
        {
            return cmt::readyFuture(createImpl<http2::server::Channel>(std::move(netStreamChannel)));
        };

        // in wsChannel(net::stream::Channel) -> ws::Channel;
        methods()->wsChannel() += sol() * [](idl::net::stream::Channel<> netStreamChannel)
        {
            return cmt::readyFuture(createImpl<ws::Channel>(std::move(netStreamChannel)));
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    Factory::~Factory()
    {
        sol().flush();
    }
}
