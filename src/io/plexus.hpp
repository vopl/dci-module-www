/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"

namespace dci::module::www::io
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    class Plexus
    {
    public:
        Plexus(idl::net::stream::Channel<> netStreamChannel);
        ~Plexus();

        template <class... InputArgs, class... OutputArgs>
        void emplace(std::tuple<InputArgs...> inputArgs, std::tuple<OutputArgs...> outputArgs) requires (!serverMode);

    public:
        void done(OutputImpl* output);
        void write(Bytes data);

    public:
        void close(primitives::ExceptionPtr e = {});
        sbs::Wire<>                                 _closed;
        sbs::Wire<void, primitives::ExceptionPtr>   _failed;

    private:
        idl::net::stream::Channel<> _netStreamChannel;
        sbs::Owner _sol;

        using InputHolder = utils::ct::If<serverMode, InputImpl, std::deque<InputImpl>>;
        InputHolder   _inputHolder;

        using OutputHolder = std::deque<OutputImpl>;
        OutputHolder  _outputHolder;

        bool _receiveStarted{};
        Bytes _receivedData;
    };
}

namespace dci::module::www::io
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    Plexus<InputImpl, OutputImpl, serverMode>::Plexus(idl::net::stream::Channel<> netStreamChannel)
        : _netStreamChannel{std::move(netStreamChannel)}
    {
        if constexpr(serverMode)
            _inputHolder.init(this);

        // in  send            (bytes);
        // out sended          (uint64 now, uint64 wait);
        // _netStreamChannel.methods()->sended() += _sol * [](uint64 now, uint64 wait)
        // {
        //     dbgFatal("not impl");
        // };

        // in  startReceive    ();

        // out received        (bytes);
        _netStreamChannel.methods()->received() += _sol * [this](Bytes data)
        {
            _receivedData.end().write(std::move(data));

            if constexpr(serverMode)
            {
                while(!_receivedData.empty())
                {
                    if(_inputHolder.onReceived(_receivedData.begin()))
                        ;
                    else if(!_receivedData.empty())
                    {
                        close(exception::buildInstance<api::http::error::MalformedInputReceived>());
                        return;
                    }
                }
            }
            else
            {
                while(!_receivedData.empty() && !_inputHolder.empty())
                {
                    if(_inputHolder.front().onReceived(_receivedData.begin()))
                        _inputHolder.pop_front();
                    else if(!_receivedData.empty())
                    {
                        close(exception::buildInstance<api::http::error::MalformedInputReceived>());
                        return;
                    }
                }

                if(!_receivedData.empty())
                {
                    _receiveStarted = false;
                    _netStreamChannel->stopReceive();
                }
            }
        };

        // in  stopReceive     ();

        // out failed          (exception);
        _netStreamChannel.methods()->failed() += _sol * [this](primitives::ExceptionPtr exception)
        {
            close(exception::buildInstance<api::http::error::DownstreamFailed>(std::move(exception)));
        };

        // out closed          ();
        _netStreamChannel.methods()->closed() += _sol * [this]()
        {
            _closed.in();
        };

        if(serverMode || _receiveStarted)
            _netStreamChannel.methods()->startReceive();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    Plexus<InputImpl, OutputImpl, serverMode>::~Plexus()
    {
        close();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    template <class... InputArgs, class... OutputArgs>
    void Plexus<InputImpl, OutputImpl, serverMode>::emplace(std::tuple<InputArgs...> inputArgs, std::tuple<OutputArgs...> outputArgs) requires (!serverMode)
    {
        {
            std::apply([&](auto&&... args)
                {
                    _inputHolder.emplace_back(this, std::forward<decltype(args)>(args)...);
                },
                std::move(inputArgs));

            if(!_receiveStarted)
            {
                _receiveStarted = true;
                _netStreamChannel.methods()->startReceive();
            }
        }

        {
            std::apply([&](auto&&... args)
                {
                    _outputHolder.emplace_back(this, std::forward<decltype(args)>(args)...);
                },
                std::move(outputArgs));

            _outputHolder.front().allowWrite();
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    void Plexus<InputImpl, OutputImpl, serverMode>::write(Bytes data)
    {
        _netStreamChannel->send(std::move(data));
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    void Plexus<InputImpl, OutputImpl, serverMode>::done(OutputImpl* /*output*/)
    {
        dbgAssert(!_outputHolder.empty());

        while(!_outputHolder.empty())
        {
            _outputHolder.front().allowWrite();
            if(_outputHolder.front().isDone())
                _outputHolder.pop_front();
            else
                break;
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    void Plexus<InputImpl, OutputImpl, serverMode>::close(ExceptionPtr e)
    {
        if(e)
            _failed.in(std::move(e));

        if(_netStreamChannel)
            _netStreamChannel->close();

        _sol.flush();

        if constexpr(serverMode)
            ;
        else
            _inputHolder.clear();
        _outputHolder.clear();

        _receiveStarted = false;
        _receivedData.clear();
    }
}
