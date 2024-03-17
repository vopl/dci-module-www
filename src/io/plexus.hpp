/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "io/inputBase.hpp"
#include "io/outputBase.hpp"

namespace dci::module::www::io
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    class Plexus
    {
    public:
        Plexus(idl::net::stream::Channel<> netStreamChannel);
        ~Plexus();

        void reg(InputMDC* input);
        void done(InputMDC* input);
        void unreg(InputMDC* input);
        void reg(OutputMDC* output);
        void done(OutputMDC* output);
        void unreg(OutputMDC* output);

        template <class... InputArgs, class... OutputArgs>
        void emplace(std::tuple<InputArgs...> inputArgs, std::tuple<OutputArgs...> outputArgs);

    public:
        void write(Bytes data);

    public:
        void close();
        sbs::Wire<>                                 _closed;
        sbs::Wire<void, primitives::ExceptionPtr>   _failed;

    private:
        idl::net::stream::Channel<> _netStreamChannel;
        sbs::Owner _sol;

        std::deque<InputMDC>     _queueInput;
        std::deque<OutputMDC>    _queueOutput;
    };
}

namespace dci::module::www::io
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    Plexus<InputMDC, OutputMDC>::Plexus(idl::net::stream::Channel<> netStreamChannel)
        : _netStreamChannel{std::move(netStreamChannel)}
    {
        // in  send            (bytes);
        // out sended          (uint64 now, uint64 wait);
        // _netStreamChannel.methods()->sended() += _sol * [](uint64 now, uint64 wait)
        // {
        //     dbgFatal("not impl");
        // };

        // in  startReceive    ();
        // out received        (bytes);
        _netStreamChannel.methods()->received() += _sol * [](Bytes /*data*/)
        {
            dbgFatal("not impl");
        };

        // in  stopReceive     ();

        // out failed          (exception);
        _netStreamChannel.methods()->failed() += _sol * [this](primitives::ExceptionPtr exception)
        {
            dbgFatal("not impl");
            _failed.in(std::move(exception));
        };

        // out closed          ();
        _netStreamChannel.methods()->closed() += _sol * [this]()
        {
            dbgFatal("not impl");
            _closed.in();
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    Plexus<InputMDC, OutputMDC>::~Plexus()
    {
        _sol.flush();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    void Plexus<InputMDC, OutputMDC>::reg(InputMDC* input)
    {
        (void)input;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    void Plexus<InputMDC, OutputMDC>::done(InputMDC* input)
    {
        dbgFatal("not impl");
        (void)input;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    void Plexus<InputMDC, OutputMDC>::unreg(InputMDC* input)
    {
        (void)input;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    void Plexus<InputMDC, OutputMDC>::reg(OutputMDC* output)
    {
        (void)output;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    void Plexus<InputMDC, OutputMDC>::done(OutputMDC* output)
    {
        dbgFatal("not impl");
        (void)output;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    void Plexus<InputMDC, OutputMDC>::unreg(OutputMDC* output)
    {
        (void)output;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    template <class... InputArgs, class... OutputArgs>
    void Plexus<InputMDC, OutputMDC>::emplace(std::tuple<InputArgs...> inputArgs, std::tuple<OutputArgs...> outputArgs)
    {
        std::apply([&](auto&&... args)
            {
                _queueInput.emplace_back(this, std::forward<decltype(args)>(args)...);
            },
            std::move(inputArgs));

        std::apply([&](auto&&... args)
            {
                _queueOutput.emplace_back(this, std::forward<decltype(args)>(args)...);
            },
            std::move(outputArgs));
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    void Plexus<InputMDC, OutputMDC>::write(Bytes data)
    {
        _netStreamChannel->send(std::move(data));
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class InputMDC, class OutputMDC>
    void Plexus<InputMDC, OutputMDC>::close()
    {
        _netStreamChannel->close();
    }
}
