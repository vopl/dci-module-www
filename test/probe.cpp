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

namespace
{
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

        auto connected = netHost->streamClient()->connect(*netServer->localEndpoint());

        return
        {
            *wwwFactory->httpClientChannel(*connected),
            *wwwFactory->httpServerChannel(*accepted)
        };
    }
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, probe)
{
    auto [cln, srv] = connectedPair();

    sbs::Owner sol;
    cmt::Event srvFirstLineDone, srvHeadersDone, srvDataDone;

    primitives::List<www::http::Header> srvHeaders;
    Bytes srvData;

    // server request
    srv->io() += sol * [&](www::http::server::Request<> srvReq, www::http::server::Response<> /*srvResp*/)
    {
        srvReq->firstLine() += sol * [&](www::http::firstLine::Method method, dci::primitives::String&& path, www::http::firstLine::Version version)
        {
            EXPECT_EQ(method, www::http::firstLine::Method::GET);
            EXPECT_EQ(path, "/");
            EXPECT_EQ(version, www::http::firstLine::Version::HTTP_1_1);
            srvFirstLineDone.raise();
        };

        srvReq->headers() += sol * [&](primitives::List<www::http::Header>&& headers, bool done)
        {
            srvHeaders.insert(srvHeaders.end(), headers.begin(), headers.end());
            if(!done)
                return;

            ASSERT_EQ(2, srvHeaders.size());

            EXPECT_EQ(srvHeaders[0].key, www::http::header::KeyRecognized::Host);
            EXPECT_EQ(srvHeaders[0].value, "localhost");

            EXPECT_EQ(srvHeaders[1].key, "x-my-header");
            EXPECT_EQ(srvHeaders[1].value, "x-my-value");

            srvHeadersDone.raise();
        };

        srvReq->data() += sol * [&](Bytes&& data, bool done)
        {
            srvData.end().write(std::move(data));
            if(!done)
                return;

            EXPECT_EQ(data, Bytes{"xyz"});
            EXPECT_TRUE(done);
            srvDataDone.raise();
        };
    };

    // client request
    {
        www::http::client::Request<> clnReq;
        www::http::client::Response<> clnResp;
        cln->io(clnReq.init2(), clnResp.init2());

        clnReq->firstLine(www::http::firstLine::Method::GET, "/", www::http::firstLine::Version::HTTP_1_1);
        clnReq->headers(
                    primitives::List<www::http::Header> {
                        {www::http::header::KeyRecognized::Host, "localhost"},
                        {"x-my-header", "x-my-value"},
                    }, true);
        clnReq->data("xyz", true);
        clnReq->done();
    }

    auto wres = cmt::wait(poll::timeout(std::chrono::seconds{1}) || (srvFirstLineDone && srvHeadersDone && srvDataDone));
    EXPECT_EQ(wres.to_string(), "0111");
    sol.flush();
}
