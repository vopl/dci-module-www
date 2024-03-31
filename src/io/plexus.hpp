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
        sbs::Owner& sol();

    public:
        Plexus(idl::net::stream::Channel<> netStreamChannel);
        ~Plexus();

        template <class... InputArgs, class... OutputArgs>
        void emplace(std::tuple<InputArgs...> inputArgs, std::tuple<OutputArgs...> outputArgs) requires (!serverMode);

        template <class... OutputArgs>
        void emplace(OutputArgs&&... outputArgs) requires (serverMode);

    public:
        void done(OutputImpl* output);
        void write(Bytes data);

    public:
        void close(primitives::ExceptionPtr e = {});
        sbs::Wire<>                                 _closed;
        sbs::Wire<void, primitives::ExceptionPtr>   _failed;

        sbs::Owner _sol;

    private:
        idl::net::stream::Channel<> _netStreamChannel;

        using InputHolder = utils::ct::If<serverMode, InputImpl, std::deque<InputImpl>>;
        InputHolder   _inputHolder;

        using OutputHolder = std::deque<OutputImpl>;
        OutputHolder  _outputHolder;

        bool _receiveStarted{};
        Bytes _receivedData;
    };
}

#include "plexus.ipp"
