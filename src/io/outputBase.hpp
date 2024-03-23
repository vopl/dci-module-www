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
    template <class Support_, class Impl>
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

        bool isDone();
        void allowWrite();

    protected:
        void flushBuffer();
        void apiDone();

    protected:
        Support* _support;
        Bytes    _buffer;
        bool     _writeAllowed{};
        bool     _apiDone{};
    };
}

namespace dci::module::www::io
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support_, class Impl>
    OutputBase<Support_, Impl>::OutputBase(Support* support)
        : _support{support}
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support_, class Impl>
    OutputBase<Support_, Impl>::~OutputBase()
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support_, class Impl>
    bool OutputBase<Support_, Impl>::isDone()
    {
        return _apiDone && _writeAllowed && _buffer.empty();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support_, class Impl>
    void OutputBase<Support_, Impl>::allowWrite()
    {
        _writeAllowed = true;
        flushBuffer();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support_, class Impl>
    void OutputBase<Support_, Impl>::flushBuffer()
    {
        if(_writeAllowed && !_buffer.empty())
        {
            _support->write(std::move(_buffer));
            dbgAssert(_buffer.empty());
            if(isDone())
                _support->done(static_cast<Impl*>(this));
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support_, class Impl>
    void OutputBase<Support_, Impl>::apiDone()
    {
        if(!_apiDone)
        {
            _apiDone = true;
            if(isDone())
                _support->done(static_cast<Impl*>(this));
        }
    }
}
