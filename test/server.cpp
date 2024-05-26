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

namespace testing::internal
{
    template <>
    void PrintTo<dci::Bytes>(const dci::Bytes& value, ::std::ostream* os)
    {
        *os << value.toString();
    }
}

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

    struct IO
    {
        www::http::server::Request<>    _input;
        www::http::server::Response<>   _output;
    };
    std::deque<IO>                  _ios;
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

            /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
            input->failed() += _sol * [&](dci::primitives::ExceptionPtr&& e)
            {
                _actions.emplace_back(InputFailed{dci::exception::toString(e)});
            };

            input->closed() += _sol * [&]()
            {
                _actions.emplace_back(InputClosed{});
            };

            input->firstLine() += _sol * [&](www::http::firstLine::Method method, primitives::String&& uri, www::http::firstLine::Version version)
            {
                _actions.emplace_back(InputFirstLine{ method, std::move(uri), version});
            };

            input->headers() += _sol * [&](primitives::List<www::http::Header>&& headers, bool done)
            {
                _actions.emplace_back(InputHeaders{ std::move(headers), done});
            };

            input->data() += _sol * [&](Bytes&& data, bool done)
            {
                _actions.emplace_back(InputData{ std::move(data), done});
            };

            input->done() += _sol * [&]()
            {
                _actions.emplace_back(InputDone{});
            };

            /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
            output->failed() += _sol * [&](dci::primitives::ExceptionPtr&& e)
            {
                _actions.emplace_back(OutputFailed{dci::exception::toString(e)});
            };

            output->closed() += _sol * [&]()
            {
                _actions.emplace_back(OutputClosed{});
            };

            _ios.emplace_back(IO{std::move(input), std::move(output)});
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

        _peer->startReceive();
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

#define CHECK_IO()                                      \
    ASSERT_GT(spectacle._actions.size(), 1);            \
    ASSERT_EQ(spectacle._actions[0], Spectacle::Io{});

#define CHECK_FAIL_ONE(direction, failType)                                                                                         \
    ASSERT_TRUE(spectacle.has<Spectacle::direction##Failed>());                                                                     \
    ASSERT_EQ(spectacle.get<Spectacle::direction##Failed>().get<0>(), "dci::idl::gen::www::http::error::request::" #failType "{}"); \
    ASSERT_TRUE(spectacle.has<Spectacle::direction##Failed>());                                                                     \
    ASSERT_LT(spectacle.pos<Spectacle::direction##Failed>(), spectacle.pos<Spectacle:: direction##Closed>());

#define CHECK_FAIL(failType)        \
    CHECK_FAIL_ONE(, failType)      \
    CHECK_FAIL_ONE(Input, failType) \
    CHECK_FAIL_ONE(Output, failType)

#define CHECK_PEERDATA(data)                                                                \
    ASSERT_TRUE(spectacle.has<Spectacle::PeerData>());                                      \
    ASSERT_EQ(spectacle.get<Spectacle::PeerData>().get<0>(), data);                         \
    ASSERT_TRUE(spectacle.has<Spectacle::PeerClosed>());                                    \
    ASSERT_LT(spectacle.pos<Spectacle::PeerData>(), spectacle.pos<Spectacle::PeerClosed>());

#define PLAY_2_FAIL(req, err, resp) \
    {                               \
        Spectacle spectacle;        \
        spectacle._peer->send(req); \
        spectacle.play();           \
        CHECK_IO();                 \
        CHECK_FAIL(err);            \
        CHECK_PEERDATA(resp);       \
    };


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
    PLAY_2_FAIL("01234567890123456789012345678901e", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET_____________________________e", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET_\t__\v_\b___\0_____________________e", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_badMethod)
{
    PLAY_2_FAIL("TEG uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("TE uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("T uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("XYZ uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("1 uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL(" uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("# uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("! uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("\330 uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("\n uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("( uri HTTP/1.1\r\n", BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bigUri)
{
    PLAY_2_FAIL("METH " + std::string(8193, 'x'), TooBigUri, "HTTP/1.1 414 URI Too Long\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bigVersion)
{
    PLAY_2_FAIL("GET uri HTTP/012345678901\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_badRequestByVersion)
{
    PLAY_2_FAIL("GET uri \t\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri 0123\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri \x01\xd4\x10\x21\x02\x02\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTZ/1.1\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri http/1.1\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri http/1.1\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_badVersion)
{
    PLAY_2_FAIL("GET uri HTTP/\x00\x01\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/\x01\x21\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/\x02\x01\x01\x01\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/\x07\x07\x07\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");

    PLAY_2_FAIL("GET uri HTTP/0.1\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.4\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/17.1\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/2.1\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/3.1\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");

    // PLAY_2_FAIL("GET uri HTTP/0.9\r\n\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    // PLAY_2_FAIL("GET uri HTTP/1.0\r\n\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    // PLAY_2_FAIL("GET uri HTTP/1.1\r\n\r\n", BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_badHeaderKey)
{
    PLAY_2_FAIL("GET uri HTTP/1.1\r\n" + std::string(65, 'x') + ": xyz\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    PLAY_2_FAIL("GET uri HTTP/1.1\r\n: xyz\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nxyz\r\n:\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nhea der: xyz\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nh e a d e r: xyz\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nheader\x00: xyz\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\n\x00header: xyz\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\n\x00: xyz\r\n", BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_badHeaderValue)
{
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nh:" + std::string(8193, 'v') + "\r\n", TooBigHeaders, "HTTP/1.1 431 Request Header Fields Too Large\r\nConnection: close\r\n\r\n");

    {
        std::string hdr = "h:" + std::string(4096, 'v') + "\r\n";
        PLAY_2_FAIL("GET uri HTTP/1.1\r\n" + hdr + hdr + hdr + hdr + hdr + hdr + hdr + hdr + "\r\n", TooBigHeaders, "HTTP/1.1 431 Request Header Fields Too Large\r\nConnection: close\r\n\r\n");
    }
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_failChain)
{
        Spectacle spectacle;
        spectacle._peer->send("GET uri HTTP/1.1\r\n\r\n");
        spectacle.play();
        spectacle._peer->send("GET uri HTTP/1.1\r\n\r\n");
        spectacle.play();
        spectacle._peer->send("GET uri HTTP/1.1\r\n\r\n");
        spectacle.play();
        spectacle._peer->send("GET uri HTTP/1.1\r\n\r\n");
        spectacle.play();

        spectacle._peer->send("bad req uest\r\n");
        spectacle.play();

        int k = 220;
}
