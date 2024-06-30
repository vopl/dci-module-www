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
    void Headers::Current::reset()
    {
        _key.reset();
        _value.reset();
        _kind = {};
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    bool Headers::Сonveyor::canDetachSome() const
    {
        if(_allowLastValueContinue)
            return _tail.size() > 1;

        return !_tail.empty();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    primitives::List<api::http::Header> Headers::Сonveyor::detachSome()
    {
        if(_allowLastValueContinue)
        {
            if(_tail.size() < 2)
                return {};

            primitives::List<api::http::Header> toDetach;
            toDetach.swap(_tail);

            _tail.emplace_back(std::move(toDetach.front()));
            toDetach.pop_front();

            return std::exchange(toDetach, {});
        }

        return std::exchange(_tail, {});
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    bool Body::needDecompress()
    {
        return !_decompressor.holds<compress::None>();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    std::optional<Bytes> Body::decompress(Bytes&& content, bool finish)
    {
        return _decompressor.visit([&](auto& concrete)
        {
            return concrete.exec(std::move(content), finish);
        });
    }
}
