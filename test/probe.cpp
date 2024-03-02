/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include <dci/test.hpp>
#include <dci/host.hpp>
#include <dci/poll.hpp>
#include "www.hpp"

using namespace dci;
using namespace dci::host;
using namespace dci::cmt;
using namespace dci::idl;

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, probe)
{
    Manager* manager = testManager();
    net::Host<> netHost = manager->createService<net::Host<>>().value();
    net::stream::Server<> netServer = netHost->streamServer().value();
    net::stream::Client<> netClient = netHost->streamClient().value();

    netServer->listen(net::Ip4Endpoint{{127,0,0,19}, 0}).value();

    www::Factory<> wwwFactory = manager->createService<www::Factory<>>().value();

    dci::sbs::Owner tol;
    www::http::server::Channel<> httpServerChannel;
    netServer->accepted().connect(tol, [&](net::stream::Channel<> netStreamChannel)
    {
        httpServerChannel = wwwFactory->httpServerChannel(std::move(netStreamChannel)).value();
    });

    www::http::client::Channel<> netClientChannel = netClient->connect(netServer->localEndpoint().value()).value();
    www::http::server::Channel<> httpClientChannel = wwwFactory->httpClientChannel(std::move(netClientChannel)).value();




}
