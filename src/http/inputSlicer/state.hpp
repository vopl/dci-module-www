/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "accumuler.hpp"

namespace dci::module::www::http::inputSlicer::state
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    struct RequestNull
    {
    };

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    struct RequestFirstLine
    {
        Accumuler<std::array<char, 32>> _method;
        Accumuler<std::string, 8192>    _uri;
        Accumuler<std::array<char, 16>> _version;

        void reset();
    };

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    struct ResponseNull
    {
    };

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    struct ResponseFirstLine
    {
        Accumuler<std::array<char, 16>> _version;
        std::uint16_t                   _statusCode{};
        std::uint16_t                   _statusCodeCharsCount{};
        Accumuler<std::string, 64>      _statusText;

        void reset();
    };

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    struct Header
    {
        Accumuler<std::array<char, 64>> _key;
        Accumuler<std::string, 8192>    _value;

        enum class Kind
        {
            unknown,
            regular,
            valueContinue,
            empty
        } _kind{};

        void reset();
    };
    constexpr std::size_t _maxEntityHeaders{256};

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    struct Body
    {
        bool _trailersFollows{};

        void reset();
    };
}
