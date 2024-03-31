/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include "pch.hpp"
#include "inputSlicer.hpp"
#include "inputSlicer/accumuleUntil.hpp"

namespace dci::module::www::http
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    InputSlicer<mode, Derived>::InputSlicer() requires (inputSlicer::Mode::request == mode)
        : _procesor{&InputSlicer::requestNull}
        , _activeState{ActiveState::requestNull}
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    InputSlicer<mode, Derived>::InputSlicer() requires (inputSlicer::Mode::response == mode)
        : _procesor{&InputSlicer::responseNull}
        , _activeState{ActiveState::responseNull}
    {
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    InputSlicer<mode, Derived>::~InputSlicer()
    {
        state<inputSlicer::state::RequestNull, false>().~RequestNull();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::process(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::Result res = (this->*_procesor)(sa);
        while(!sa.empty() && inputSlicer::Result::needMoreInput == res)
            res = (this->*_procesor)(sa);

        return res;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::requestNull(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode)
    {
        state<inputSlicer::state::RequestFirstLine, false>().reset();
        _procesor = &InputSlicer::requestFirstLineMethod;
        return requestFirstLineMethod(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::requestFirstLineMethod(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode)
    {
        inputSlicer::Result result = inputSlicer::accumuleUntil<' '>(sa, state<inputSlicer::state::RequestFirstLine>()._method);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::requestFirstLineUri;
        return requestFirstLineUri(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::requestFirstLineUri(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode)
    {
        inputSlicer::Result result = inputSlicer::accumuleUntil<' '>(sa, state<inputSlicer::state::RequestFirstLine>()._uri);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::requestFirstLineVersion;
        return requestFirstLineVersion(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::requestFirstLineVersion(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode)
    {
        inputSlicer::Result result = inputSlicer::accumuleUntil<'\r'>(sa, state<inputSlicer::state::RequestFirstLine>()._version);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::firstLineCR;
        return firstLineCR(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::responseNull(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::response == mode)
    {
        state<inputSlicer::state::ResponseFirstLine, false>().reset();
        _procesor = &InputSlicer::responseFirstLineVersion;
        return responseFirstLineVersion(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::responseFirstLineVersion(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::response == mode)
    {
        inputSlicer::Result result = inputSlicer::accumuleUntil<' '>(sa, state<inputSlicer::state::ResponseFirstLine>()._version);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::responseFirstLineStatusCode;
        return responseFirstLineStatusCode(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::responseFirstLineStatusCode(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::response == mode)
    {
        inputSlicer::state::ResponseFirstLine& stateResponseFirstLine = state<inputSlicer::state::ResponseFirstLine>();
        while(stateResponseFirstLine._statusCodeCharsCount < 4)
        {
            if(sa.empty())
                return inputSlicer::Result::needMoreInput;

            char c = sa.front();

            if(stateResponseFirstLine._statusCodeCharsCount < 3)
            {
                if('9' < c || '0' > c)
                    return inputSlicer::Result::malformedInput;

                sa.dropFront(1);
                stateResponseFirstLine._statusCode *= 10;
                stateResponseFirstLine._statusCode += c - '0';
                ++stateResponseFirstLine._statusCodeCharsCount;
            }
            else
            {
                if(' ' != c)
                    return inputSlicer::Result::malformedInput;

                sa.dropFront(1);
                ++stateResponseFirstLine._statusCodeCharsCount;
                break;
            }
        }

        _procesor = &InputSlicer::responseFirstLineStatusText;
        return responseFirstLineStatusText(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::responseFirstLineStatusText(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::response == mode)
    {
        inputSlicer::Result result = inputSlicer::accumuleUntil<'\r'>(sa, state<inputSlicer::state::ResponseFirstLine>()._statusText);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::firstLineCR;
        return firstLineCR(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::firstLineCR(inputSlicer::SourceAdapter& sa)
    {
        if(sa.empty())
            return inputSlicer::Result::needMoreInput;

        if('\n' != sa.front())
            return inputSlicer::Result::malformedInput;

        sa.dropFront(1);

        inputSlicer::Result result;
        if constexpr(inputSlicer::Mode::request == mode)
            result = static_cast<Derived*>(this)->sliceDone(state<inputSlicer::state::RequestFirstLine>());
        else
            result = static_cast<Derived*>(this)->sliceDone(state<inputSlicer::state::ResponseFirstLine>());

        if(inputSlicer::Result::done != result)
            return result;

        state<inputSlicer::state::Header, false>().reset();
        _procesor = &InputSlicer::headerKey;
        return headerKey(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerKey(inputSlicer::SourceAdapter& sa)
    {
        if(!sa.empty() && '\r' == sa.front())
        {
            sa.dropFront(1);
            state<inputSlicer::state::Header>()._empty = true;
            _procesor = &InputSlicer::headerCR;
            return headerCR(sa);
        }

        inputSlicer::Result result = inputSlicer::accumuleUntil<':'>(sa, state<inputSlicer::state::Header>()._key);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::headerPreValue;
        return headerPreValue(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerPreValue(inputSlicer::SourceAdapter& sa)
    {
        while(!sa.empty())
        {
            if(' ' == sa.front())
                sa.dropFront(1);
            else
            {
                _procesor = &InputSlicer::headerValue;
                return headerValue(sa);
            }
        }
        return inputSlicer::Result::needMoreInput;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerValue(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::Result result = inputSlicer::accumuleUntil<'\r'>(sa, state<inputSlicer::state::Header>()._value);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::headerCR;
        return headerCR(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerCR(inputSlicer::SourceAdapter& sa)
    {
        if(sa.empty())
            return inputSlicer::Result::needMoreInput;

        if('\n' != sa.front())
            return inputSlicer::Result::malformedInput;

        sa.dropFront(1);

        inputSlicer::state::Header& stateHeader = state<inputSlicer::state::Header>();

        inputSlicer::Result result = static_cast<Derived*>(this)->sliceDone(stateHeader);
        if(inputSlicer::Result::done != result)
            return result;

        if(stateHeader._empty)
        {
            state<inputSlicer::state::Body, false>().reset();
            state<inputSlicer::state::Body>()._trailersFollows = false;
            _procesor = &InputSlicer::body;
            return body(sa);
        }

        stateHeader.reset();
        _procesor = &InputSlicer::headerKey;
        return headerKey(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::body(inputSlicer::SourceAdapter& sa)
    {
        sa.dropFront(sa.segmentSize());

        inputSlicer::Result result = static_cast<Derived*>(this)->sliceDone(state<inputSlicer::state::Body>());
        if(inputSlicer::Result::done != result)
            return result;

        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    template <class S, bool optimistic>
    S& InputSlicer<mode, Derived>::state()
    {
        if constexpr(optimistic)
        {
            if constexpr(std::is_same_v<S, inputSlicer::state::RequestNull>)        {dbgAssert(ActiveState::requestNull       == _activeState); return _requestNull;        }
            if constexpr(std::is_same_v<S, inputSlicer::state::RequestFirstLine>)   {dbgAssert(ActiveState::requestFirstLine  == _activeState); return _requestFirstLine;   }
            if constexpr(std::is_same_v<S, inputSlicer::state::ResponseNull>)       {dbgAssert(ActiveState::responseNull      == _activeState); return _responseNull;       }
            if constexpr(std::is_same_v<S, inputSlicer::state::ResponseFirstLine>)  {dbgAssert(ActiveState::responseFirstLine == _activeState); return _responseFirstLine;  }
            if constexpr(std::is_same_v<S, inputSlicer::state::Header>)             {dbgAssert(ActiveState::header            == _activeState); return _header;             }
            if constexpr(std::is_same_v<S, inputSlicer::state::Body>)               {dbgAssert(ActiveState::body              == _activeState); return _body;               }
        }
        else
        {
            switch(_activeState)
            {
            case ActiveState::requestNull:
                if constexpr(std::is_same_v<S, inputSlicer::state::RequestNull>)        return _requestNull;
                _requestNull.~RequestNull();
                break;
            case ActiveState::requestFirstLine:
                if constexpr(std::is_same_v<S, inputSlicer::state::RequestFirstLine>)   return _requestFirstLine;
                _requestFirstLine.~RequestFirstLine();
                break;
            case ActiveState::responseNull:
                if constexpr(std::is_same_v<S, inputSlicer::state::ResponseNull>)       return _responseNull;
                _responseNull.~ResponseNull();
                break;
            case ActiveState::responseFirstLine:
                if constexpr(std::is_same_v<S, inputSlicer::state::ResponseFirstLine>)  return _responseFirstLine;
                _responseFirstLine.~ResponseFirstLine();
                break;
            case ActiveState::header:
                if constexpr(std::is_same_v<S, inputSlicer::state::Header>)             return _header;
                _header.~Header();
                break;
            case ActiveState::body:
                if constexpr(std::is_same_v<S, inputSlicer::state::Body>)               return _body;
                _body.~Body();
                break;
            }

            if constexpr(std::is_same_v<S, inputSlicer::state::RequestNull>)
            {
                _activeState = ActiveState::requestNull;
                return *(new (&_requestNull) S);
            }

            if constexpr(std::is_same_v<S, inputSlicer::state::RequestFirstLine>)
            {
                _activeState = ActiveState::requestFirstLine;
                return *(new (&_requestFirstLine) S);
            }

            if constexpr(std::is_same_v<S, inputSlicer::state::ResponseNull>)
            {
                _activeState = ActiveState::responseNull;
                return *(new (&_responseNull) S);
            }

            if constexpr(std::is_same_v<S, inputSlicer::state::ResponseFirstLine>)
            {
                _activeState = ActiveState::responseFirstLine;
                return *(new (&_responseFirstLine) S);
            }

            if constexpr(std::is_same_v<S, inputSlicer::state::Header>)
            {
                _activeState = ActiveState::header;
                return *(new (&_header) S);
            }

            if constexpr(std::is_same_v<S, inputSlicer::state::Body>)
            {
                _activeState = ActiveState::body;
                return *(new (&_body) S);
            }
        }

        unreacheable();
        static S s;
        return s;
    }
}
