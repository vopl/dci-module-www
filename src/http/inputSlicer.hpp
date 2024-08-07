/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "inputSlicer/mode.hpp"
#include "inputSlicer/result.hpp"
#include "inputSlicer/sourceAdapter.hpp"
#include "inputSlicer/state.hpp"

namespace dci::module::www::http
{
    template <inputSlicer::Mode mode, class Derived>
    struct InputSlicer
    {
    protected:
        InputSlicer() requires (inputSlicer::Mode::request == mode);
        InputSlicer() requires (inputSlicer::Mode::response == mode);
        ~InputSlicer();

        void reset();
        inputSlicer::Result process(inputSlicer::SourceAdapter& sa);

    protected:
        inputSlicer::Result sliceStart();
        inputSlicer::Result sliceFlush(inputSlicer::state::RequestFirstLine& firstLine);
        inputSlicer::Result sliceFlush(inputSlicer::state::Headers& headers, bool done);
        inputSlicer::Result sliceFlush(inputSlicer::state::Body& body, bool done);

    private:
        using Processor = inputSlicer::Result (InputSlicer::*)(inputSlicer::SourceAdapter& sa);

    private:
        inputSlicer::Result requestNull(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode);

        inputSlicer::Result requestFirstLineMethod(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode);
        inputSlicer::Result requestFirstLineUri(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode);
        inputSlicer::Result requestFirstLineVersion(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode);

        inputSlicer::Result responseNull(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::response == mode);

        inputSlicer::Result responseFirstLineVersion(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::response == mode);
        inputSlicer::Result responseFirstLineStatusCode(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::response == mode);
        inputSlicer::Result responseFirstLineStatusText(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::response == mode);

        inputSlicer::Result firstLineLF(inputSlicer::SourceAdapter& sa);

        inputSlicer::Result headerPreKey(inputSlicer::SourceAdapter& sa);
        inputSlicer::Result headerKey(inputSlicer::SourceAdapter& sa);
        inputSlicer::Result headerPreValue(inputSlicer::SourceAdapter& sa);
        inputSlicer::Result headerValue(inputSlicer::SourceAdapter& sa);
        inputSlicer::Result headerLF(inputSlicer::SourceAdapter& sa);

        inputSlicer::Result bodyUntilClose(inputSlicer::SourceAdapter& sa);
        inputSlicer::Result bodyByContentLength(inputSlicer::SourceAdapter& sa);
        inputSlicer::Result bodyChunked(inputSlicer::SourceAdapter& sa);

    private:
        Processor _procesor;

    private:
        enum class ActiveState
        {
            requestNull,
            requestFirstLine,
            responseNull,
            responseFirstLine,
            headers,
            bodyUntilClose,
            bodyByContentLength,
            bodyChunked
        } _activeState;

        union
        {
            inputSlicer::state::RequestNull         _requestNull;
            inputSlicer::state::RequestFirstLine    _requestFirstLine;
            inputSlicer::state::ResponseNull        _responseNull;
            inputSlicer::state::ResponseFirstLine   _responseFirstLine;
            inputSlicer::state::Headers             _headers;
            inputSlicer::state::BodyUntilClose      _bodyUntilClose;
            inputSlicer::state::BodyByContentLength _bodyByContentLength;
            inputSlicer::state::BodyChunked         _bodyChunked;
        };

        template <class S, bool optimistic = true>
        S& state();

        template <class S>
        const S& state() const;
    };
}

#include "inputSlicer.ipp"
