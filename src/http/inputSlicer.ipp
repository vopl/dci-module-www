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
#include "../enumSupport.hpp"

namespace
{
    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inline bool isspace(char c)
    {
        switch(c)
        {
        case ' ':
        case '\t':
            return true;
        }

        return false;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inline std::string& ltrim(std::string& s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char c){return !isspace(c);}));
        return s;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inline std::string&& ltrim(std::string&& s)
    {
        return std::move(ltrim(s));
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inline std::string& rtrim(std::string& s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](char c){return !isspace(c);}).base(), s.end());
        return s;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inline std::string&& rtrim(std::string&& s)
    {
        return std::move(rtrim(s));
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inline std::string& trim(std::string& s)
    {
        return ltrim(rtrim(s));
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    inline std::string&& trim(std::string&& s)
    {
        return std::move(trim(s));
    }
}

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
    void InputSlicer<mode, Derived>::reset()
    {
        if constexpr(inputSlicer::Mode::request == mode)
        {
            _procesor = &InputSlicer::requestNull;
            state<inputSlicer::state::RequestNull, false>();
        }
        else if constexpr(inputSlicer::Mode::response == mode)
        {
            _procesor = &InputSlicer::responseNull;
            state<inputSlicer::state::ResponseNull, false>();
        }
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::process(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::Result res = (this->*_procesor)(sa);
        while(!sa.empty() && inputSlicer::Result::needMore == res)
            res = (this->*_procesor)(sa);

        return res;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    bool InputSlicer<mode, Derived>::hasDetacheableHeadersAccumuled() const
    {
        if(ActiveState::headers != _activeState)
            return false;

        const inputSlicer::state::Headers& stateHeaders = state<inputSlicer::state::Headers>();

        if(stateHeaders._allowValueContinue)
            return stateHeaders._accumuled.size() > 1;

        return !stateHeaders._accumuled.empty();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    primitives::List<api::http::Header> InputSlicer<mode, Derived>::detachHeadersAccumuled()
    {
        if(ActiveState::headers != _activeState)
            return {};

        inputSlicer::state::Headers& stateHeaders = state<inputSlicer::state::Headers>();

        if(stateHeaders._allowValueContinue)
        {
            if(stateHeaders._accumuled.size() < 2)
                return {};

            primitives::List<api::http::Header> detach;
            detach.swap(stateHeaders._accumuled);

            stateHeaders._accumuled.emplace_back(std::move(detach.front()));
            detach.pop_front();

            return std::exchange(detach, {});
        }

        return std::exchange(stateHeaders._accumuled, {});
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    bool InputSlicer<mode, Derived>::hasDetacheableBodyAccumuled() const
    {
        if(ActiveState::body != _activeState)
            return {};

        const inputSlicer::state::Body& stateBody = state<inputSlicer::state::Body>();

        return !stateBody._accumuled.empty();
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    Bytes InputSlicer<mode, Derived>::detachBodyAccumuled()
    {
        if(ActiveState::body != _activeState)
            return {};

        inputSlicer::state::Body& stateBody = state<inputSlicer::state::Body>();

        return std::exchange(stateBody._accumuled, {});
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::sliceStart()
    {
        //empty is okay
        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::sliceDone(inputSlicer::state::RequestFirstLine& firstLine)
    {
        firstLine._parsedMethod = enumSupport::toEnum<api::http::firstLine::Method>(firstLine._method.str());
        if(!firstLine._parsedMethod)
            return inputSlicer::Result::badMethod;

        if( firstLine._version._size < 5 ||
            firstLine._version._downstream[0] != 'H' ||
            firstLine._version._downstream[1] != 'T' ||
            firstLine._version._downstream[2] != 'T' ||
            firstLine._version._downstream[3] != 'P' ||
            firstLine._version._downstream[4] != '/'
        )
            return inputSlicer::Result::badEntity;

        firstLine._parsedVersion = enumSupport::toEnum<api::http::firstLine::Version>(firstLine._version.str());
        if(!firstLine._parsedVersion)
            return inputSlicer::Result::badVersion;

        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::sliceDone(inputSlicer::state::Headers& headers)
    {
        switch(headers._current._kind)
        {
        case inputSlicer::state::Headers::Current::Kind::unknown:
            dbgAssert("unparsed header declated as sliced");
            return inputSlicer::Result::badEntity;

        case inputSlicer::state::Headers::Current::Kind::regular:
            {
                if(inputSlicer::state::_maxEntityHeaders <= headers._accumuled.size())
                    return inputSlicer::Result::tooBigHeaders;

                if(!headers._current._key._size)
                    return inputSlicer::Result::badEntity;

                for(std::size_t i{}; i<headers._current._key._size; ++i)
                {
                    char c = headers._current._key._downstream[i];
                    switch(c)
                    {
                    case '(':
                    case ')':
                    case '<':
                    case '>':
                    case '@':
                    case ',':
                    case ';':
                    case ':':
                    case '\\':
                    case '"':
                    case '/':
                    case '[':
                    case ']':
                    case '?':
                    case '=':
                    case '{':
                    case '}':
                        return inputSlicer::Result::badEntity;
                    default:
                        if(32 >= c || 127 == c)
                            return inputSlicer::Result::badEntity;
                    }
                }

                rtrim(headers._current._value._downstream);
                headers._cumulativeValueSize += headers._current._value._downstream.size();
                if(inputSlicer::state::_maxEntityHeaderValueSize <= headers._cumulativeValueSize)
                    return inputSlicer::Result::tooBigHeaders;

                std::optional<api::http::header::KeyRecognized> keyRecognized = enumSupport::toEnum<api::http::header::KeyRecognized>(headers._current._key.str());
                if(keyRecognized)
                {
                    switch(*keyRecognized)
                    {
                    case api::http::header::KeyRecognized::Content_Length:
                        headers._allowValueContinue = false;
                        dbgFatal("not impl");
                        break;
                    case api::http::header::KeyRecognized::Content_Encoding:
                        headers._allowValueContinue = false;
                        dbgFatal("not impl");
                        break;
                    case api::http::header::KeyRecognized::Transfer_Encoding:
                        headers._allowValueContinue = false;
                        dbgFatal("not impl");
                        break;
                    case api::http::header::KeyRecognized::Content_Transfer_Encoding:
                        headers._allowValueContinue = false;
                        dbgFatal("not impl");
                        break;
                    case api::http::header::KeyRecognized::Connection:
                        headers._allowValueContinue = false;
                        dbgFatal("not impl");
                        break;
                    case api::http::header::KeyRecognized::Trailer:
                        headers._allowValueContinue = false;
                        dbgFatal("not impl");
                        break;
                    default:
                        headers._allowValueContinue = true;
                        break;
                    }

                    headers._accumuled.emplace_back(*keyRecognized, std::move(headers._current._value._downstream));
                }
                else
                {
                    headers._allowValueContinue = true;
                    headers._accumuled.emplace_back(api::http::header::KeyAny{headers._current._key.str()}, std::move(headers._current._value._downstream));
                }
            }
            return inputSlicer::Result::needMore;

        case inputSlicer::state::Headers::Current::Kind::valueContinue:
            {
                dbgAssert(headers._current._key.empty());
                if(!headers._allowValueContinue || headers._accumuled.empty())
                    return inputSlicer::Result::badEntity;

                std::string& value = headers._accumuled.back().value;
                std::string addition = trim(std::move(headers._current._value._downstream));

                std::size_t totalValueSize = value.size() + 1 + addition.size();
                if(totalValueSize > decltype(headers._current._value)::_limit)
                    return inputSlicer::Result::tooBigHeaders;

                value.reserve(totalValueSize);
                value += ' ';
                value += std::move(addition);
            }
            return inputSlicer::Result::needMore;

        case inputSlicer::state::Headers::Current::Kind::empty:
            return inputSlicer::Result::done;
        }

        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::sliceDone(inputSlicer::state::Body& /*body*/)
    {
        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::requestNull(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode)
    {
        inputSlicer::Result result = static_cast<Derived*>(this)->sliceStart();
        if(inputSlicer::Result::done != result)
            return result;

        state<inputSlicer::state::RequestFirstLine, false>();
        _procesor = &InputSlicer::requestFirstLineMethod;
        return requestFirstLineMethod(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::requestFirstLineMethod(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode)
    {
        inputSlicer::Result result = inputSlicer::accumuleUntil<' ', inputSlicer::Result::badEntity>(sa, state<inputSlicer::state::RequestFirstLine>()._method);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::requestFirstLineUri;
        return requestFirstLineUri(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::requestFirstLineUri(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode)
    {
        inputSlicer::Result result = inputSlicer::accumuleUntil<' ', inputSlicer::Result::tooBigUri>(sa, state<inputSlicer::state::RequestFirstLine>()._uri);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::requestFirstLineVersion;
        return requestFirstLineVersion(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::requestFirstLineVersion(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::request == mode)
    {
        inputSlicer::Result result = inputSlicer::accumuleUntil<'\r', inputSlicer::Result::badEntity>(sa, state<inputSlicer::state::RequestFirstLine>()._version);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::firstLineCR;
        return firstLineCR(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::responseNull(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::response == mode)
    {
        inputSlicer::Result result = static_cast<Derived*>(this)->sliceSart();
        if(inputSlicer::Result::done != result)
            return result;

        state<inputSlicer::state::ResponseFirstLine, false>().reset();
        _procesor = &InputSlicer::responseFirstLineVersion;
        return responseFirstLineVersion(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::responseFirstLineVersion(inputSlicer::SourceAdapter& sa) requires (inputSlicer::Mode::response == mode)
    {
        inputSlicer::Result result = inputSlicer::accumuleUntil<' ', inputSlicer::Result::badEntity>(sa, state<inputSlicer::state::ResponseFirstLine>()._version);
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
                return inputSlicer::Result::needMore;

            char c = sa.front();

            if(stateResponseFirstLine._statusCodeCharsCount < 3)
            {
                if('9' < c || '0' > c)
                    return inputSlicer::Result::badStatus;

                sa.dropFront(1);
                stateResponseFirstLine._statusCode *= 10;
                stateResponseFirstLine._statusCode += c - '0';
                ++stateResponseFirstLine._statusCodeCharsCount;
            }
            else
            {
                if(!isspace(c))
                    return inputSlicer::Result::badStatus;

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
        inputSlicer::Result result = inputSlicer::accumuleUntil<'\r', inputSlicer::Result::badEntity>(sa, state<inputSlicer::state::ResponseFirstLine>()._statusText);
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
            return inputSlicer::Result::needMore;

        if('\n' != sa.front())
            return inputSlicer::Result::badEntity;

        sa.dropFront(1);

        inputSlicer::Result result;
        if constexpr(inputSlicer::Mode::request == mode)
            result = static_cast<Derived*>(this)->sliceDone(state<inputSlicer::state::RequestFirstLine>());
        else
            result = static_cast<Derived*>(this)->sliceDone(state<inputSlicer::state::ResponseFirstLine>());

        if(inputSlicer::Result::done == result)
        {
            state<inputSlicer::state::Headers, false>();
            _procesor = &InputSlicer::headerPreKey;
            return headerPreKey(sa);
        }

        return result;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerPreKey(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::state::Headers& stateHeaders = state<inputSlicer::state::Headers>();
        dbgAssert(inputSlicer::state::Headers::Current::Kind::unknown == stateHeaders._current._kind);

        if(sa.empty())
            return inputSlicer::Result::needMore;

        char c = sa.front();

        if('\r' == c)
        {
            sa.dropFront(1);
            stateHeaders._current._kind = inputSlicer::state::Headers::Current::Kind::empty;
            stateHeaders._allowValueContinue = false;
            _procesor = &InputSlicer::headerCR;
            return headerCR(sa);
        }

        if(isspace(c))
        {
            if(!stateHeaders._allowValueContinue)
                return inputSlicer::Result::badEntity;

            sa.dropFront(1);
            stateHeaders._current._kind = inputSlicer::state::Headers::Current::Kind::valueContinue;
            _procesor = &InputSlicer::headerPreValue;
            return headerPreValue(sa);
        }

        stateHeaders._current._kind = inputSlicer::state::Headers::Current::Kind::regular;
        _procesor = &InputSlicer::headerKey;
        return headerKey(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerKey(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::state::Headers& stateHeaders = state<inputSlicer::state::Headers>();
        dbgAssert(inputSlicer::state::Headers::Current::Kind::regular == stateHeaders._current._kind);

        if(sa.empty())
            return inputSlicer::Result::needMore;

        inputSlicer::Result result = inputSlicer::accumuleUntil<':', inputSlicer::Result::badEntity>(sa, stateHeaders._current._key);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::headerPreValue;
        return headerPreValue(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerPreValue(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::state::Headers& stateHeaders = state<inputSlicer::state::Headers>();
        dbgAssert(inputSlicer::state::Headers::Current::Kind::regular == stateHeaders._current._kind ||
                  inputSlicer::state::Headers::Current::Kind::valueContinue == stateHeaders._current._kind);

        while(!sa.empty())
        {
            if(isspace(sa.front()))
                sa.dropFront(1);
            else
            {
                _procesor = &InputSlicer::headerValue;
                return headerValue(sa);
            }
        }
        return inputSlicer::Result::needMore;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerValue(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::state::Headers& stateHeaders = state<inputSlicer::state::Headers>();
        dbgAssert(inputSlicer::state::Headers::Current::Kind::regular == stateHeaders._current._kind ||
                  inputSlicer::state::Headers::Current::Kind::valueContinue == stateHeaders._current._kind);

        inputSlicer::Result result = inputSlicer::accumuleUntil<'\r', inputSlicer::Result::tooBigHeaders>(sa, stateHeaders._current._value);
        if(inputSlicer::Result::done != result)
            return result;

        _procesor = &InputSlicer::headerCR;
        return headerCR(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerCR(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::state::Headers& stateHeaders = state<inputSlicer::state::Headers>();
        dbgAssert(inputSlicer::state::Headers::Current::Kind::unknown != stateHeaders._current._kind);

        if(sa.empty())
            return inputSlicer::Result::needMore;

        if('\n' != sa.front())
            return inputSlicer::Result::badEntity;

        sa.dropFront(1);

        inputSlicer::Result result = static_cast<Derived*>(this)->sliceDone(stateHeaders);
        switch(result)
        {
        case inputSlicer::Result::needMore:
            stateHeaders._current.reset();
            _procesor = &InputSlicer::headerPreKey;
            return headerPreKey(sa);

        case inputSlicer::Result::done:
            {
                inputSlicer::state::BodyRelatedHeaders brh = std::move(stateHeaders);
                state<inputSlicer::state::Body, false>() = std::move(brh);
                _procesor = &InputSlicer::body;
                return body(sa);
            }

        default:
            break;
        }

        return result;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::body(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::state::Body& stateBody = state<inputSlicer::state::Body>();

        dbgFatal("decode/accumule body");
        sa.dropFront(sa.segmentSize());

        inputSlicer::Result result = static_cast<Derived*>(this)->sliceDone(stateBody);

        if(inputSlicer::Result::done == result && stateBody._trailersFollows)
        {
            dbgFatal("not impl");
            // state<inputSlicer::state::Headers, false>().reset();
            // _procesor = &InputSlicer::headerPreKey;
            // return headerPreKey(sa);
        }

        return result;
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
            if constexpr(std::is_same_v<S, inputSlicer::state::Headers>)            {dbgAssert(ActiveState::headers           == _activeState); return _headers;            }
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
            case ActiveState::headers:
                if constexpr(std::is_same_v<S, inputSlicer::state::Headers>)            return _headers;
                _headers.~Headers();
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

            if constexpr(std::is_same_v<S, inputSlicer::state::Headers>)
            {
                _activeState = ActiveState::headers;
                return *(new (&_headers) S);
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

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    template <class S>
    const S& InputSlicer<mode, Derived>::state() const
    {
        if constexpr(std::is_same_v<S, inputSlicer::state::RequestNull>)        {dbgAssert(ActiveState::requestNull       == _activeState); return _requestNull;        }
        if constexpr(std::is_same_v<S, inputSlicer::state::RequestFirstLine>)   {dbgAssert(ActiveState::requestFirstLine  == _activeState); return _requestFirstLine;   }
        if constexpr(std::is_same_v<S, inputSlicer::state::ResponseNull>)       {dbgAssert(ActiveState::responseNull      == _activeState); return _responseNull;       }
        if constexpr(std::is_same_v<S, inputSlicer::state::ResponseFirstLine>)  {dbgAssert(ActiveState::responseFirstLine == _activeState); return _responseFirstLine;  }
        if constexpr(std::is_same_v<S, inputSlicer::state::Headers>)            {dbgAssert(ActiveState::headers           == _activeState); return _headers;            }
        if constexpr(std::is_same_v<S, inputSlicer::state::Body>)               {dbgAssert(ActiveState::body              == _activeState); return _body;               }

        unreacheable();
        static S s;
        return s;
    }

}
