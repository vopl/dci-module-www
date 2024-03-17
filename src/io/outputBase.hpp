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
    template <class Support_, class MDC>
    class OutputBase
    {
    public:
        using Support = Support_;

    protected:
        OutputBase(Support* support);
        ~OutputBase();

    public:
        void onFailed(primitives::ExceptionPtr) = delete;
        void onClosed() = delete;

    protected:
        Support* _support;
    };
}

namespace dci::module::www::io
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support_, class MDC>
    OutputBase<Support_, MDC>::OutputBase(Support* support)
        : _support{support}
    {
        _support->reg(static_cast<MDC*>(this));
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support_, class MDC>
    OutputBase<Support_, MDC>::~OutputBase()
    {
        _support->unreg(static_cast<MDC*>(this));
    }
}
