/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "state.hpp"

namespace dci::module::www::http::inputSlicer::state
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void RequestFirstLine::reset()
    {
        _method.reset();
        _uri.reset();
        _version.reset();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void ResponseFirstLine::reset()
    {
        _version.reset();
        _statusCode = {};
        _statusCodeCharsCount = {};
        _statusText.reset();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Header::reset()
    {
        _key.reset();
        _value.reset();
        _kind = {};
        // _empty = {};
        // _valueContinue = {};
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    void Body::reset()
    {
        _trailersFollows = {};
    }
}
