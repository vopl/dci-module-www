/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "accumuleUntil.hpp"

namespace dci::module::www::http::inputSlicer
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <char terminator, class Accumuler>
    Result accumuleUntil(SourceAdapter& source, Accumuler& accumuler)
    {
        while(!source.empty())
        {
            std::size_t availSize = std::min(source.segmentSize(), Accumuler::_limit - accumuler.size() + 1);
            auto availBegin = source.segmentBegin();
            auto availEnd = availBegin+availSize;
            auto foundIter = std::find(availBegin, availEnd, terminator);

            std::size_t size4Accumule = foundIter - availBegin;

            if(Accumuler::_limit < accumuler.size() + size4Accumule)
                return Result::malformedInput;

            accumuler.append(availBegin, foundIter);

            if(availEnd == foundIter)
                source.dropFront(size4Accumule);
            else
            {
                source.dropFront(size4Accumule+1);
                return Result::done;
            }
        }

        return Result::needMoreInput;
    }
}
