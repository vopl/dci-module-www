/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once
#include <dci/host/module/entry.hpp>
#include <dci/cmt.hpp>
#include <dci/exception.hpp>
#include <dci/poll/timeout.hpp>
#include <dci/utils/atScopeExit.hpp>
#include <dci/utils/overloaded.hpp>
#include <dci/utils/compiler.hpp>

#include <zlib.h>

#include <bit>
#include <deque>
#include <string_view>
#include "www.hpp"

namespace dci::module::www
{
    namespace api = dci::idl::www;
}

