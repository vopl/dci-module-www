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
    Base<Support, Impl, Api>::Base()
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    Base<Support, Impl, Api>::Base(Support* support, Api&& api)
        : _support{support}
        , _api{std::move(api)}
    {
        dbgAssert(_api);

        // in close();
        _api.methods()->close() += _sol * [this]()
        {
            _support->apiWantClose(static_cast<Impl*>(this));
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    Base<Support, Impl, Api>::~Base()
    {
        _sol.flush();
        _api.reset();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    void Base<Support, Impl, Api>::setSupport(Support* support)
    {
        dbgAssert(!_support);
        dbgAssert(support);

        _support = support;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    void Base<Support, Impl, Api>::setApi(Api&& api)
    {
        dbgAssert(!_api);
        dbgAssert(api);

        _api = api;

        // in close();
        _api.methods()->close() += _sol * [this]()
        {
            _support->apiWantClose(static_cast<Impl*>(this));
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    void Base<Support, Impl, Api>::fireFailed(primitives::ExceptionPtr e)
    {
        if(_api)
            _api->failed(std::move(e));
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <class Support, class Impl, class Api>
    void Base<Support, Impl, Api>::fireClosed(bool andReset)
    {
        if(andReset)
            _sol.flush();

        if(_api)
        {
            _api->closed();

            if(andReset)
                _api.reset();
        }
    }
}
