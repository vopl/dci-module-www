/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "base.hpp"

namespace dci::module::www::io
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    class OutputBase : public Base<Support, Impl, Api>
    {
    protected:
        OutputBase(Support* support, Api&& api);
        ~OutputBase();

    public:
        bool isFail();
        bool isDone();
        void allowWrite();

    protected:
        void flushBuffer();
        void apiDone();
        void fail();

    protected:
        Bytes    _buffer;
        bool     _writeAllowed{};
        bool     _apiDone{};
        bool     _fail{};
    };
}

#include "outputBase.ipp"
