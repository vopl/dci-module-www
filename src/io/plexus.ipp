/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "plexus.hpp"

namespace dci::module::www::io
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    sbs::Owner& Plexus<InputImpl, OutputImpl, serverMode>::sol()
    {
        return _sol;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    Plexus<InputImpl, OutputImpl, serverMode>::Plexus(idl::net::stream::Channel<> netStreamChannel)
        : _netStreamChannel{std::move(netStreamChannel)}
    {
        if constexpr(serverMode)
            _inputHolder.setSupport(this);

        // in  send            (bytes);
        // out sended          (uint64 now, uint64 wait);
        // _netStreamChannel->sended() += _sol * [](uint64 now, uint64 wait)
        // {
        //     dbgFatal("not impl");
        // };

        // in  startReceive    ();

        // out received        (bytes);
        _netStreamChannel->received() += _sol * [this](Bytes data)
        {
            _receivedData.end().write(std::move(data));

            if constexpr(serverMode)
            {
                while(!_receivedData.empty())
                {
                    bool onReceiveResult;
                    {
                        bytes::Alter receivedDataAlter = _receivedData.begin();
                        onReceiveResult = _inputHolder.onReceived(receivedDataAlter);
                    }

                    if(onReceiveResult)
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
        _netStreamChannel->failed() += _sol * [this](primitives::ExceptionPtr exception)
        {
            close(exception::buildInstance<api::http::error::DownstreamFailed>(std::move(exception)));
        };

        // out closed          ();
        _netStreamChannel->closed() += _sol * [this]()
        {
            close();
        };

        if(serverMode || _receiveStarted)
            _netStreamChannel->startReceive();
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
                _netStreamChannel->startReceive();
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

    template <class InputImpl, class OutputImpl, bool serverMode>
    template <class... OutputArgs>
    void Plexus<InputImpl, OutputImpl, serverMode>::emplace(OutputArgs&&... outputArgs) requires (serverMode)
    {
        _outputHolder.emplace_back(this, std::forward<OutputArgs>(outputArgs)...);
        _outputHolder.front().allowWrite();
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
    void Plexus<InputImpl, OutputImpl, serverMode>::write(Bytes data)
    {
        _netStreamChannel->send(std::move(data));
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    void Plexus<InputImpl, OutputImpl, serverMode>::close(ExceptionPtr e)
    {
        _sol.flush();

        if(_netStreamChannel)
            _netStreamChannel->close();

        _receiveStarted = false;
        _receivedData.clear();

        if constexpr(serverMode)
        {
            if(e)
                _inputHolder.onFailed(e);
            _inputHolder.onClosed();
        }
        else
        {
            for(InputImpl& inputImpl : _inputHolder)
            {
                if(e)
                    inputImpl.onFailed(e);
                inputImpl.onClosed();
            }

            _inputHolder.clear();
        }

        for(OutputImpl& outputImpl : _outputHolder)
        {
            if(e)
                outputImpl.onFailed(e);
            outputImpl.onClosed();
        }
        _outputHolder.clear();

        if(e)
            _failed.in(std::move(e));

        _closed.in();
    }
}
