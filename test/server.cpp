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
#include <dci/exception.hpp>
#include <dci/utils/s2f.hpp>
#include "www.hpp"

using namespace dci;
using namespace dci::host;
using namespace dci::cmt;
using namespace dci::idl;
using namespace dci::primitives;

struct Spectacle
{
    struct Failed           : Tuple<std::string>                               { using Tuple::Tuple; };
    struct Closed           : Tuple<>                                          { using Tuple::Tuple; };
    struct Io               : Tuple<>                                          { using Tuple::Tuple; };

    struct InputFailed      : Tuple<std::string>                               { using Tuple::Tuple; };
    struct InputClosed      : Tuple<>                                          { using Tuple::Tuple; };
    struct InputFirstLine   : Tuple<www::http::firstLine::Method, primitives::String, www::http::firstLine::Version> { using Tuple::Tuple; };
    struct InputHeaders     : Tuple<primitives::List<www::http::Header>, bool> { using Tuple::Tuple; };
    struct InputData        : Tuple<Bytes, bool>                               { using Tuple::Tuple; };
    struct InputDone        : Tuple<>                                          { using Tuple::Tuple; };

    struct OutputFailed     : Tuple<std::string>                               { using Tuple::Tuple; };
    struct OutputClosed     : Tuple<>                                          { using Tuple::Tuple; };

    struct PeerFailed       : Tuple<std::string>                               { using Tuple::Tuple; };
    struct PeerClosed       : Tuple<>                                          { using Tuple::Tuple; };
    struct PeerData         : Tuple<Bytes>                                     { using Tuple::Tuple; };

    using Action = primitives::Variant
    <
        Failed,
        Closed,
        Io,

        InputFailed,
        InputClosed,
        InputFirstLine,
        InputHeaders,
        InputData,
        InputDone,

        OutputFailed,
        OutputClosed,

        PeerFailed,
        PeerClosed,
        PeerData
    >;

    std::deque<Action>              _actions;
    www::http::server::Channel<>    _target;
    www::http::server::Request<>    _input;
    www::http::server::Response<>   _output;
    net::stream::Channel<>          _peer;
    sbs::Owner                      _sol;

    Spectacle(www::http::server::Channel<>&& target, net::stream::Channel<>&& peer)
        : _target(std::move(target))
        , _peer(std::move(peer))
    {
        interconnect();
    }

    Spectacle()
    {
        Manager* manager = testManager();
        net::Host<> netHost = *manager->createService<net::Host<>>();
        www::Factory<> wwwFactory = *manager->createService<www::Factory<>>();

        net::stream::Server<> netServer = *netHost->streamServer();
        *netServer->listen(net::Ip4Endpoint{{127,0,0,19}, 0});
        utils::S2f accepted = netServer->accepted();

        _peer = *netHost->streamClient()->connect(*netServer->localEndpoint());
        _target = *wwwFactory->httpServerChannel(*accepted);

        interconnect();
    }

    ~Spectacle()
    {
        _sol.flush();
    }

    void interconnect()
    {
        _target->failed() += _sol * [&](dci::primitives::ExceptionPtr&& e)
        {
            _actions.emplace_back(Failed{dci::exception::toString(e)});
        };

        _target->closed() += _sol * [&]()
        {
            _actions.emplace_back(Closed{});
        };

        _target->io() += _sol * [&](www::http::server::Request<>&& input, www::http::server::Response<>&& output)
        {
            _actions.emplace_back(Io{});

            _input = std::move(input);
            _output = std::move(output);

            /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
            _input->failed() += _sol * [&](dci::primitives::ExceptionPtr&& e)
            {
                _actions.emplace_back(InputFailed{dci::exception::toString(e)});
            };

            _input->closed() += _sol * [&]()
            {
                _actions.emplace_back(InputClosed{});
            };

            _input->firstLine() += _sol * [&](www::http::firstLine::Method method, primitives::String&& uri, www::http::firstLine::Version version)
            {
                _actions.emplace_back(InputFirstLine{ method, std::move(uri), version});
            };

            _input->headers() += _sol * [&](primitives::List<www::http::Header>&& headers, bool done)
            {
                _actions.emplace_back(InputHeaders{ std::move(headers), done});
            };

            _input->data() += _sol * [&](Bytes&& data, bool done)
            {
                _actions.emplace_back(InputData{ std::move(data), done});
            };

            _input->done() += _sol * [&]()
            {
                _actions.emplace_back(InputDone{});
            };

            /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
            _output->failed() += _sol * [&](dci::primitives::ExceptionPtr&& e)
            {
                _actions.emplace_back(OutputFailed{dci::exception::toString(e)});
            };

            _output->closed() += _sol * [&]()
            {
                _actions.emplace_back(OutputClosed{});
            };
        };

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        _peer->failed() += _sol * [&](dci::primitives::ExceptionPtr&& e)
        {
            _actions.emplace_back(PeerFailed{dci::exception::toString(e)});
        };

        _peer->closed() += _sol * [&]()
        {
            _actions.emplace_back(PeerClosed{});
        };

        _peer->received() += _sol * [&](Bytes&& data)
        {
            _actions.emplace_back(PeerData{ std::move(data) });
        };
    }

    void play()
    {
        int countDown = 10;
        for(; countDown>=0; --countDown)
        {
            std::size_t actionsSize = _actions.size();
            poll::timeout(std::chrono::milliseconds{1}).wait();
            if(_actions.size() != actionsSize)
                countDown = 10;
        }
    }

    template <class A>
    bool has() const
    {
        for(const Action& a : _actions)
            if(a.holds<A>())
                return true;
        return false;
    }

    template <class A>
    const A& get() const
    {
        for(const Action& a : _actions)
            if(a.holds<A>())
                return a.get<A>();
        throw "no data";
    }

    template <class A>
    std::size_t pos() const
    {
        std::size_t res{};
        for(const Action& a : _actions)
        {
            if(a.holds<A>())
                return res;
            ++res;
        }
        throw "no data";
    }
};

#define CONCAT(a, b) a ## b

#define CHECK_IO() \
    ASSERT_GT(spectacle._actions.size(), 1); \
    ASSERT_EQ(spectacle._actions[0], Spectacle::Io{});

#define CHECK_FAIL_ONE(k, failType) \
    ASSERT_TRUE(spectacle.has<Spectacle:: CONCAT(k, Failed)>()); \
    ASSERT_EQ(spectacle.get<Spectacle:: CONCAT(k, Failed)>().get<0>(), "dci::idl::gen::www::http::error::request::" #failType "{}"); \
    ASSERT_TRUE(spectacle.has<Spectacle:: CONCAT(k, Closed)>()); \
    ASSERT_LT(spectacle.pos<Spectacle:: CONCAT(k, Failed)>(), spectacle.pos<Spectacle:: CONCAT(k, Closed)>());

#define CHECK_FAIL(failType) \
    CHECK_FAIL_ONE(, failType) \
    CHECK_FAIL_ONE(Input, failType) \
    CHECK_FAIL_ONE(Output, failType)

#define CHECK_PEERDATA(data) \
    ASSERT_TRUE(spectacle.has<Spectacle::PeerData>()); \
    ASSERT_EQ(spectacle.get<Spectacle::PeerData>().get<0>(), data); \
    ASSERT_TRUE(spectacle.has<Spectacle::PeerClosed>()); \
    ASSERT_LT(spectacle.pos<Spectacle::PeerData>(), spectacle.pos<Spectacle::PeerClosed>());

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_noInput)
{
    Spectacle spectacle;
    spectacle._peer->send("");
    spectacle.play();

    ASSERT_EQ(spectacle._actions.size(), 0);
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_smallMethod)
{
    auto sample = [](Bytes traffic)
    {
        Spectacle spectacle;
        spectacle._peer->send(traffic);
        spectacle.play();

        ASSERT_EQ(spectacle._actions.size(), 1);
        ASSERT_EQ(spectacle._actions[0], Spectacle::Io{});
    };

    sample("x");
    sample("0123456789");
    sample("01234567890123456789012345678901");// less then 32
    sample("GET_____________________________");// less then 32
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bigMethod)
{
    auto sample = [](Bytes traffic)
    {
        Spectacle spectacle;
        spectacle._peer->send(traffic);
        spectacle.play();

        CHECK_IO();
        CHECK_FAIL(BadRequest);
        CHECK_PEERDATA("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    };

    sample("01234567890123456789012345678901e");
    sample("GET_____________________________e");
    sample("GET_\t__\v_\b___\0_____________________e");
}
