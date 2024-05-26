/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "plexus.hpp"
#include "../channelSoftClosing.hpp"

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
    Plexus<InputImpl, OutputImpl, serverMode>::Plexus(idl::net::stream::Channel<>&& netStreamChannel, idl::www::Unreliable<>::Opposite&& unreliableOpposite)
        : _netStreamChannel{std::move(netStreamChannel)}
        , _unreliableOpposite{std::move(unreliableOpposite)}
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

            auto stopReceive = [&]
            {
                if(_receiveStarted && _netStreamChannel)
                {
                    _receiveStarted = false;
                    _netStreamChannel->stopReceive();
                }
            };

            if(InputProcessResult::bad == _inputProcessResult)
            {
                stopReceive();
                return;
            }

            if constexpr(serverMode)
            {
                while(!_receivedData.empty())
                {
                    {
                        bytes::Alter receivedDataAlter = _receivedData.begin();
                        _inputProcessResult = _inputHolder.process(receivedDataAlter);
                    }

                    switch(_inputProcessResult)
                    {
                    case InputProcessResult::needMore:
                    case InputProcessResult::done:
                        break;

                    case InputProcessResult::bad:
                        stopReceive();
                        return;
                    }
                }
            }
            else
            {
                dbgFatal("not impl");
                // while(!_receivedData.empty() && !_inputHolder.empty())
                // {
                //     if(_inputHolder.front().process(_receivedData.begin()))
                //         _inputHolder.pop_front();
                //     else if(!_receivedData.empty())
                //     {
                //         close(exception::buildInstance<api::http::error::response::BadResponse>());
                //         return;
                //     }
                // }

                // if(!_receivedData.empty())
                // {
                //     _receiveStarted = false;
                //     _netStreamChannel->stopReceive();
                // }
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

        // in close();
        _unreliableOpposite->close() += _sol * [&]()
        {
            close();
        };

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
        _inputHolder.setResponse(&_outputHolder.front());
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
            {
                if(_outputHolder.front().isFail())
                {
                    close();
                    break;
                }
                _outputHolder.pop_front();
            }
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
    void Plexus<InputImpl, OutputImpl, serverMode>::closeInput(ExceptionPtr /*e*/)
    {
        if(_netStreamChannel)
            _netStreamChannel->shutdown(true, false);

        _receiveStarted = false;
        _receivedData.clear();

        dbgFatal("not impl");
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputImpl, class OutputImpl, bool serverMode>
    void Plexus<InputImpl, OutputImpl, serverMode>::close(ExceptionPtr e)
    {
        _sol.flush();

        if(_netStreamChannel)
            ChannelSoftClosing::instance().push(std::exchange(_netStreamChannel, {}));

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

        if(_unreliableOpposite)
        {
            if(e)
                _unreliableOpposite->failed(std::move(e));

            _unreliableOpposite->closed();
        }
    }
}
