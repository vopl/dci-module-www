/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "outputBase.hpp"

namespace dci::module::www::io
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    OutputBase<Support, Impl, Api>::OutputBase(Support* support, Api&& api)
        : Base<Support, Impl, Api>{support, std::move(api)}
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    OutputBase<Support, Impl, Api>::~OutputBase()
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    bool OutputBase<Support, Impl, Api>::isDone()
    {
        return _apiDone && _writeAllowed && _buffer.empty();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    void OutputBase<Support, Impl, Api>::allowWrite()
    {
        _writeAllowed = true;
        flushBuffer();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    void OutputBase<Support, Impl, Api>::flushBuffer()
    {
        if(_writeAllowed && !_buffer.empty())
        {
            this->_support->write(std::move(_buffer));
            dbgAssert(_buffer.empty());
            if(isDone())
                this->_support->done(static_cast<Impl*>(this));
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    void OutputBase<Support, Impl, Api>::apiDone()
    {
        if(!_apiDone)
        {
            _apiDone = true;
            if(isDone())
                this->_support->done(static_cast<Impl*>(this));
        }
    }
}
