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

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
namespace testing::internal
{
    template <>
    void PrintTo<dci::Bytes>(const dci::Bytes& value, ::std::ostream* os)
    {
        *os << value.toString();
    }
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
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

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
#define CHECK_IO()                                      \
    ASSERT_GT(spectacle._actions.size(), 1);            \
    ASSERT_EQ(spectacle._actions[0], Spectacle::Io{});

#define CHECK_FAIL_ONE(direction, failType)                                                                                         \
    ASSERT_TRUE(spectacle.has<Spectacle::direction##Failed>());                                                                     \
    ASSERT_EQ(spectacle.get<Spectacle::direction##Failed>().get<0>(), "dci::idl::gen::www::http::error::" #failType "{}"); \
    ASSERT_TRUE(spectacle.has<Spectacle::direction##Failed>());                                                                     \
    ASSERT_LT(spectacle.pos<Spectacle::direction##Failed>(), spectacle.pos<Spectacle:: direction##Closed>());

#define CHECK_FAIL(failType)        \
    CHECK_FAIL_ONE(, BadInput)      \
    CHECK_FAIL_ONE(Input, failType) \
    CHECK_FAIL_ONE(Output, BadInput)

#define CHECK_PEERDATA(data)                                                                \
    ASSERT_TRUE(spectacle.has<Spectacle::PeerData>());                                      \
    ASSERT_EQ(spectacle.get<Spectacle::PeerData>().get<0>(), data);                         \
    ASSERT_TRUE(spectacle.has<Spectacle::PeerClosed>());                                    \
    ASSERT_LT(spectacle.pos<Spectacle::PeerData>(), spectacle.pos<Spectacle::PeerClosed>());

#define CHECK_INPUTDATA(data)                                                                \
    ASSERT_TRUE(spectacle.has<Spectacle::InputData>());                                      \
    ASSERT_EQ(spectacle.get<Spectacle::InputData>().get<0>(), data);                         \
    ASSERT_TRUE(spectacle.has<Spectacle::InputDone>());                                      \
    ASSERT_TRUE(spectacle.has<Spectacle::InputClosed>());                                    \
    ASSERT_LT(spectacle.pos<Spectacle::InputData>(), spectacle.pos<Spectacle::InputClosed>());

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
    PLAY_2_FAIL("01234567890123456789012345678901e",        request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET_____________________________e",        request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET_\t__\v_\b___\0_____________________e", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_badMethod)
{
    PLAY_2_FAIL("TEG uri HTTP/1.1\r\n", request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("TE uri HTTP/1.1\r\n",  request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("T uri HTTP/1.1\r\n",   request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("XYZ uri HTTP/1.1\r\n", request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("1 uri HTTP/1.1\r\n",   request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL(" uri HTTP/1.1\r\n",    request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("# uri HTTP/1.1\r\n",   request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("! uri HTTP/1.1\r\n",   request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("\330 uri HTTP/1.1\r\n",request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("\n uri HTTP/1.1\r\n",  request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("( uri HTTP/1.1\r\n",   request::BadMethod, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bigUri)
{
    PLAY_2_FAIL("METH " + std::string(8193, 'x'), request::TooBigUri, "HTTP/1.1 414 URI Too Long\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bigVersion)
{
    PLAY_2_FAIL("GET uri HTTP/012345678901\r\n", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_badRequestByVersion)
{
    PLAY_2_FAIL("GET uri \t\r\n",                       request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri 0123\r\n",                     request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri \x01\xd4\x10\x21\x02\x02\r\n", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTZ/1.1\r\n",                 request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri http/1.1\r\n",                 request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri http/1.1\r\n",                 request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_badVersion)
{
    PLAY_2_FAIL("GET uri HTTP/\x00\x01\r\n",        request::BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/\x01\x21\r\n",        request::BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/\x02\x01\x01\x01\r\n",request::BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/\x07\x07\x07\r\n",    request::BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");

    PLAY_2_FAIL("GET uri HTTP/0.1\r\n", request::BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.4\r\n", request::BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/17.1\r\n",request::BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/2.1\r\n", request::BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/3.1\r\n", request::BadVersion, "HTTP/1.1 505 HTTP Version Not Supported\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_badHeaderKey)
{
    PLAY_2_FAIL("GET uri HTTP/1.1\r\n" + std::string(65, 'x') + ": xyz\r\n", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    PLAY_2_FAIL("GET uri HTTP/1.1\r\n: xyz\r\n",            request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nxyz\r\n:\r\n",         request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nhea der: xyz\r\n",     request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nh e a d e r: xyz\r\n", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nheader\x00: xyz\r\n",  request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\n\x00header: xyz\r\n",  request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
    PLAY_2_FAIL("GET uri HTTP/1.1\r\n\x00: xyz\r\n",        request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_badHeaderValue)
{
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nh:" + std::string(8193, 'v') + "\r\n", request::TooBigHeaders, "HTTP/1.1 431 Request Header Fields Too Large\r\nConnection: close\r\n\r\n");

    {
        std::string hdr = "h:" + std::string(4096, 'v') + "\r\n";
        PLAY_2_FAIL("GET uri HTTP/1.1\r\n" + hdr + hdr + hdr + hdr + hdr + hdr + hdr + hdr + "\r\n", request::TooBigHeaders, "HTTP/1.1 431 Request Header Fields Too Large\r\nConnection: close\r\n\r\n");
    }
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyUntilClose)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\n\r\n[this is a body]");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyUntilClose2)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nConnection:  close \r\n\r\n[this is a body]");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyByLength)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nContent-Length: 16\r\n\r\n[this is a body]extra");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyChunked)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n10\r\n[this is a body]\r\n0\r\nextra");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyChunked2)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n"
                          "3\r\n[th\r\n"
                          "7\r\nis is a\r\n"
                          "6\r\n body]\r\n"
                          "0\r\n"
                          "extra");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
// echo -n "[this is a body]" | compress -c | hexdump -v -e '"\\" "x" 1/1 "%02X"'
// \x1F\x9D\x90\x5B\xE8\xA0\x49\x33\x07\x04\x41\x10\x61\x40\x88\x79\x43\x26\x4F\x17

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyCompress)
{
    // Content-Encoding: compress
    // не поддерживаем сразу

    PLAY_2_FAIL("GET uri HTTP/1.1\r\nTransfer-Encoding: compress\r\n\r\n"
                "\x1F\x9D\x90\x5B\xE8\xA0\x49\x33\x07\x04\x41\x10\x61\x40\x88\x79\x43\x26\x4F\x17", request::UnprocessableContent, "HTTP/1.1 422 Unprocessable Content\r\nConnection: close\r\n\r\n");

    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: compress\r\n\r\n", request::UnprocessableContent, "HTTP/1.1 422 Unprocessable Content\r\nConnection: close\r\n\r\n");
}


/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
// echo -n "[this is a body]" | perl -MIO::Compress::RawDeflate -e 'undef $/; my ($in, $out) = (<>, undef); IO::Compress::RawDeflate::rawdeflate(\$in, \$out); print $out;' | hexdump -v -e '"\\" "x" 1/1 "%02X"'
// \x8B\x2E\xC9\xC8\x2C\x56\x00\xA2\x44\x85\xA4\xFC\x94\xCA\x58\x00

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyDeflate)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nTransfer-Encoding: deflate\r\n\r\n"
                          "\x8B\x2E\xC9\xC8\x2C\x56\x00\xA2\x44\x85\xA4\xFC\x94\xCA\x58\x00");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyDeflate2)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nContent-Encoding: deflate\r\n\r\n"
                          "\x8B\x2E\xC9\xC8\x2C\x56\x00\xA2\x44\x85\xA4\xFC\x94\xCA\x58\x00");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyDeflate3)
{
    LOGD("the following log lines are caused by a test, these are not errors");

    // extra
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: deflate\r\n\r\n"
                "\x8B\x2E\xC9\xC8\x2C\x56\x00\xA2\x44\x85\xA4\xFC\x94\xCA\x58\x00 abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    // bad after middle
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: deflate\r\n\r\n"
                "\x8B\x2E\xC9\xC8\x2C\x56 abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    // bad
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: deflate\r\n\r\n"
                "abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    LOGD("the end of the noisy test");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
// echo -n "[this is a body]" | gzip -c - | hexdump -v -e '"\\" "x" 1/1 "%02X"'
// \x1F\x8B\x08\x00\x00\x00\x00\x00\x00\x03\x8B\x2E\xC9\xC8\x2C\x56\x00\xA2\x44\x85\xA4\xFC\x94\xCA\x58\x00\xD0\x35\x3A\x02\x10\x00\x00\x00

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyGzip)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nTransfer-Encoding: gzip\r\n\r\n"
                          "\x1F\x8B\x08\x00\x00\x00\x00\x00\x00\x03\x8B\x2E\xC9\xC8\x2C\x56\x00\xA2\x44\x85\xA4\xFC\x94\xCA\x58\x00\xD0\x35\x3A\x02\x10\x00\x00\x00");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyGzip2)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nContent-Encoding: gzip\r\n\r\n"
                          "\x1F\x8B\x08\x00\x00\x00\x00\x00\x00\x03\x8B\x2E\xC9\xC8\x2C\x56\x00\xA2\x44\x85\xA4\xFC\x94\xCA\x58\x00\xD0\x35\x3A\x02\x10\x00\x00\x00");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyGzip3)
{
    LOGD("the following log lines are caused by a test, these are not errors");

    // extra
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: gzip\r\n\r\n"
                "\x1F\x8B\x08\x00\x00\x00\x00\x00\x00\x03\x8B\x2E\xC9\xC8\x2C\x56\x00\xA2\x44\x85\xA4\xFC\x94\xCA\x58\x00\xD0\x35\x3A\x02\x10\x00\x00\x00 abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    // bad after middle
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: gzip\r\n\r\n"
                "\x1F\x8B\x08\x00\x00\x00\x00\x00\x00\x03\x8B\x2E\xC9\xC8\x2C\x56\x00\xA2\x44\x85 abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    // bad
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: gzip\r\n\r\n"
                "abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    LOGD("the end of the noisy test");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
// echo -n "[this is a body]" | zstd -c - | hexdump -v -e '"\\" "x" 1/1 "%02X"'
// \x28\xB5\x2F\xFD\x04\x58\x81\x00\x00\x5B\x74\x68\x69\x73\x20\x69\x73\x20\x61\x20\x62\x6F\x64\x79\x5D\xBB\xDF\xC6\x39

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyZstd)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nTransfer-Encoding: zstd\r\n\r\n"
                          "\x28\xB5\x2F\xFD\x04\x58\x81\x00\x00\x5B\x74\x68\x69\x73\x20\x69\x73\x20\x61\x20\x62\x6F\x64\x79\x5D\xBB\xDF\xC6\x39");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyZstd2)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nContent-Encoding: zstd\r\n\r\n"
                          "\x28\xB5\x2F\xFD\x04\x58\x81\x00\x00\x5B\x74\x68\x69\x73\x20\x69\x73\x20\x61\x20\x62\x6F\x64\x79\x5D\xBB\xDF\xC6\x39");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyZstd3)
{
    LOGD("the following log lines are caused by a test, these are not errors");

    // extra
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: zstd\r\n\r\n"
                "\x28\xB5\x2F\xFD\x04\x58\x81\x00\x00\x5B\x74\x68\x69\x73\x20\x69\x73\x20\x61\x20\x62\x6F\x64\x79\x5D\xBB\xDF\xC6\x39 abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    // bad after middle
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: zstd\r\n\r\n"
                "\x28\xB5\x2F\xFD\x04\x58\x81\x00\x00\x5B\x74\x68\x69\x73\x20 abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    // bad
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: zstd\r\n\r\n"
                "abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    LOGD("the end of the noisy test");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
// echo -n "[this is a body]" | brotli -c - | hexdump -v -e '"\\" "x" 1/1 "%02X"'
// \x1F\x0F\x00\xF8\xA5\xB6\xBA\x52\x10\x45\x1A\x29\x17\x66\xDA\x29\x52

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyBrotli)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nTransfer-Encoding: br\r\n\r\n"
                          "\x1F\x0F\x00\xF8\xA5\xB6\xBA\x52\x10\x45\x1A\x29\x17\x66\xDA\x29\x52");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyBrotli2)
{
    Spectacle spectacle;
    spectacle._peer->send("GET uri HTTP/1.1\r\nContent-Encoding: br\r\n\r\n"
                          "\x1F\x0F\x00\xF8\xA5\xB6\xBA\x52\x10\x45\x1A\x29\x17\x66\xDA\x29\x52");
    spectacle.play();
    spectacle._peer->close();
    spectacle.play();

    CHECK_IO();
    CHECK_INPUTDATA("[this is a body]");
}

/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_www, server_bodyBrotli3)
{
    LOGD("the following log lines are caused by a test, these are not errors");

    // extra
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: br\r\n\r\n"
                "\x1F\x0F\x00\xF8\xA5\xB6\xBA\x52\x10\x45\x1A\x29\x17\x66\xDA\x29\x52 abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    // bad after middle
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: br\r\n\r\n"
                "\x1F\x0F\x00\xF8\xA5\xB6\xBA\x52 abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    // bad
    PLAY_2_FAIL("GET uri HTTP/1.1\r\nContent-Encoding: br\r\n\r\n"
                "abrakadabra shwabra", request::BadRequest, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");

    LOGD("the end of the noisy test");
}
