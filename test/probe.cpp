/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include <dci/test.hpp>
#include <dci/host.hpp>
#include <dci/poll.hpp>
#include <dci/cmt.hpp>
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

    dci::sbs::Owner sol;
    www::http::server::Channel<> httpServerChannel;
    netServer->accepted().connect(sol, [&](net::stream::Channel<> netStreamChannel)
    {
        httpServerChannel = wwwFactory->httpServerChannel(std::move(netStreamChannel)).value();
    });

    idl::net::stream::Channel<> netStreamChannel = netClient->connect(netServer->localEndpoint().value()).value();
    www::http::client::Channel<> httpClientChannel = wwwFactory->httpClientChannel(std::move(netStreamChannel)).value();

    www::http::client::Request<> request{idl::interface::Initializer{}};
    www::http::client::Response<> response{idl::interface::Initializer{}};
    httpClientChannel->io(request.opposite(), response.opposite());

    cmt::Event firstLineEvt;
    int firstLineCounter = 0;
    response->firstLine() += sol * [&](www::http::firstLine::Version version, primitives::uint8 statusCode, primitives::String statusText)
    {
        EXPECT_EQ(version, www::http::firstLine::Version::HTTP_1_1);
        EXPECT_EQ(statusCode, 200);
        EXPECT_EQ(statusText, "OK");
        ++firstLineCounter;
        firstLineEvt.raise();
    };

    cmt::Event headersEvt;
    std::size_t headersCounter = 0;
    response->headers() += sol * [&](primitives::List<www::http::Header> headers, bool done)
    {
        std::size_t localIdx{};
        while(headersCounter < headers.size())
        {
            switch(headersCounter)
            {
            case 0:
                EXPECT_EQ(headers[localIdx].key, www::http::header::KeyRecognized::Host);
                EXPECT_EQ(headers[localIdx].value, "localhost");
                break;
            case 1:
                EXPECT_EQ(headers[localIdx].key, "x-my-header");
                EXPECT_EQ(headers[localIdx].value, "x-my-value");
                break;
            default:
                ADD_FAILURE();
            }

            ++localIdx;
            ++headersCounter;
            EXPECT_EQ(done, headersCounter == 2);
            if(done)
                headersEvt.raise();
        }
    };

    cmt::Event dataEvt;
    std::size_t dataCounter = 0;
    response->data() += sol * [&](Bytes data, bool done)
    {
        primitives::String dataStr = data.toString();

        std::size_t localIdx{};
        while(dataCounter < dataStr.size())
        {
            switch(dataCounter)
            {
            case 0:
                EXPECT_EQ(dataStr[localIdx], 'x');
                break;
            case 1:
                EXPECT_EQ(dataStr[localIdx], 'y');
                break;
            case 2:
                EXPECT_EQ(dataStr[localIdx], 'z');
                break;
            default:
                ADD_FAILURE();
            }

            ++localIdx;
            ++dataCounter;
            EXPECT_EQ(done, dataCounter == 3);
            if(done)
                dataEvt.raise();
        }
    };

    cmt::Event doneEvt;
    std::size_t doneCounter = 0;
    response->done() += sol * [&]()
    {
        ++doneCounter;
        doneEvt.raise();
    };

    request->firstLine(www::http::firstLine::Method::GET, "/", www::http::firstLine::Version::HTTP_1_1);
    request->headers(
                primitives::List<www::http::Header> {
                    {www::http::header::KeyRecognized::Host, "localhost"},
                    {"x-my-header", "x-my-value"},
                }, true);
    request->data("xyz", true);
    request->done();

    cmt::wait(poll::timeout(std::chrono::seconds{1}) || (firstLineEvt && headersEvt && dataEvt && doneEvt));

    EXPECT_EQ(1, firstLineCounter);
    EXPECT_EQ(2, headersCounter);
    EXPECT_EQ(3, dataCounter);
    EXPECT_EQ(1, doneCounter);
}
