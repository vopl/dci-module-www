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
#include <dci/utils/s2f.hpp>
#include "www.hpp"

using namespace dci;
using namespace dci::host;
using namespace dci::cmt;
using namespace dci::idl;

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
std::tuple<
    www::http::client::Channel<>,
    www::http::server::Channel<>
> connectedPair()
{
    Manager* manager = testManager();
    net::Host<> netHost = *manager->createService<net::Host<>>();
    www::Factory<> wwwFactory = *manager->createService<www::Factory<>>();

    net::stream::Server<> netServer = *netHost->streamServer();
    *netServer->listen(net::Ip4Endpoint{{127,0,0,19}, 0});
    utils::S2f accepted = netServer->accepted();

    auto connected = *netHost->streamClient()->connect(*netServer->localEndpoint());

    return
    {
        *wwwFactory->httpClientChannel(connected),
        *wwwFactory->httpServerChannel(*accepted)
    };
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, probe)
{
    auto [cln, srv] = connectedPair();

    www::http::client::Request<> clnReq;
    www::http::client::Response<> clnResp;
    cln->io(clnReq.init2(), clnResp.init2());

    utils::S2f srvIo = srv->io();

    // client request
    {
        clnReq->firstLine(www::http::firstLine::Method::GET, "/", www::http::firstLine::Version::HTTP_1_1);
        clnReq->headers(
                    primitives::List<www::http::Header> {
                        {www::http::header::KeyRecognized::Host, "localhost"},
                        {"x-my-header", "x-my-value"},
                    }, true);
        clnReq->data("xyz", true);
        clnReq->done();
    }

    // server request
    {
        auto [srvReq, srvResp] = *srvIo;

        {
            auto [method, path, version] = *utils::S2f{srvReq->firstLine()};
            EXPECT_EQ(method, www::http::firstLine::Method::GET);
            EXPECT_EQ(path, "/");
            EXPECT_EQ(version, www::http::firstLine::Version::HTTP_1_1);
        }

        {
            primitives::List<www::http::Header> headers;
            for(;;)
            {
                auto [h, done] = *utils::S2f{srvReq->headers()};
                headers.insert(headers.end(), h.begin(), h.end());
                if(done)
                    break;
            };

            ASSERT_EQ(2, headers.size());

            EXPECT_EQ(headers[0].key, www::http::header::KeyRecognized::Host);
            EXPECT_EQ(headers[0].value, "localhost");

            EXPECT_EQ(headers[1].key, "x-my-header");
            EXPECT_EQ(headers[1].value, "x-my-value");
        }

        {
            Bytes data;
            for(;;)
            {
                auto [d, done] = *utils::S2f{srvReq->data()};
                d.begin().removeTo(data);
                if(done)
                    break;
            };

            EXPECT_EQ(data, Bytes{"xyz"});
        }

        utils::S2f{srvReq->done()}.wait();
    }
}
