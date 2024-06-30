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

using namespace std::literals;

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
    inputSlicer::Result InputSlicer<mode, Derived>::sliceStart()
    {
        //empty is okay
        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::sliceFlush(inputSlicer::state::RequestFirstLine& /*firstLine*/)
    {
        return inputSlicer::Result::done;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::sliceFlush(inputSlicer::state::Headers& /*headers*/, bool done)
    {
        return done ? inputSlicer::Result::done : inputSlicer::Result::needMore;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::sliceFlush(inputSlicer::state::Body& /*body*/, bool done)
    {
        return done ? inputSlicer::Result::done : inputSlicer::Result::needMore;
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

        _procesor = &InputSlicer::firstLineLF;
        return firstLineLF(sa);
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
        inputSlicer::SourceAdapter::ForHdr& saForHdr = sa.forHdr();

        inputSlicer::state::ResponseFirstLine& stateResponseFirstLine = state<inputSlicer::state::ResponseFirstLine>();
        while(stateResponseFirstLine._statusCodeCharsCount < 4)
        {
            if(saForHdr.empty())
                return inputSlicer::Result::needMore;

            char c = saForHdr.front();

            if(stateResponseFirstLine._statusCodeCharsCount < 3)
            {
                if('9' < c || '0' > c)
                    return inputSlicer::Result::badStatus;

                saForHdr.dropFront(1);
                stateResponseFirstLine._statusCode *= 10;
                stateResponseFirstLine._statusCode += c - '0';
                ++stateResponseFirstLine._statusCodeCharsCount;
            }
            else
            {
                if(!isspace(c))
                    return inputSlicer::Result::badStatus;

                saForHdr.dropFront(1);
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

        _procesor = &InputSlicer::firstLineLF;
        return firstLineLF(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::firstLineLF(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::SourceAdapter::ForHdr& saForHdr = sa.forHdr();

        if(saForHdr.empty())
            return inputSlicer::Result::needMore;

        if('\n' != saForHdr.front())
            return inputSlicer::Result::badEntity;

        saForHdr.dropFront(1);

        inputSlicer::Result result;
        if constexpr(inputSlicer::Mode::request == mode)
        {
            inputSlicer::state::RequestFirstLine& stateFirstLine = state<inputSlicer::state::RequestFirstLine>();

            stateFirstLine._parsedMethod = enumSupport::toEnum<api::http::firstLine::Method>(stateFirstLine._method.str());
            if(!stateFirstLine._parsedMethod)
                return inputSlicer::Result::badMethod;

            if( stateFirstLine._version._size < 5 ||
                stateFirstLine._version._downstream[0] != 'H' ||
                stateFirstLine._version._downstream[1] != 'T' ||
                stateFirstLine._version._downstream[2] != 'T' ||
                stateFirstLine._version._downstream[3] != 'P' ||
                stateFirstLine._version._downstream[4] != '/'
            )
                return inputSlicer::Result::badEntity;

            stateFirstLine._parsedVersion = enumSupport::toEnum<api::http::firstLine::Version>(stateFirstLine._version.str());
            if(!stateFirstLine._parsedVersion)
                return inputSlicer::Result::badVersion;

            result = static_cast<Derived*>(this)->sliceFlush(stateFirstLine);
        }
        else
        {
            inputSlicer::state::ResponseFirstLine& stateFirstLine = state<inputSlicer::state::ResponseFirstLine>();
            dbgFatal("not impl");
            result = static_cast<Derived*>(this)->sliceFlush(stateFirstLine);
        }

        if(inputSlicer::Result::done != result)
            return result;

        state<inputSlicer::state::Headers, false>();
        _procesor = &InputSlicer::headerPreKey;
        return headerPreKey(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerPreKey(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::SourceAdapter::ForHdr& saForHdr = sa.forHdr();

        inputSlicer::state::Headers& stateHeaders = state<inputSlicer::state::Headers>();
        dbgAssert(inputSlicer::state::Headers::Current::Kind::unknown == stateHeaders._current._kind);

        if(inputSlicer::state::_maxEntityHeadersCount <= stateHeaders._conveyor._totalHeadersCount)
            return inputSlicer::Result::tooBigHeaders;

        if(saForHdr.empty())
            return inputSlicer::Result::needMore;

        char c = saForHdr.front();

        if('\r' == c)
        {
            saForHdr.dropFront(1);
            stateHeaders._current._kind = inputSlicer::state::Headers::Current::Kind::empty;
            stateHeaders._conveyor._allowLastValueContinue = false;
            _procesor = &InputSlicer::headerLF;
            return headerLF(sa);
        }

        if(isspace(c))
        {
            if(!stateHeaders._conveyor._allowLastValueContinue)
                return inputSlicer::Result::badEntity;

            saForHdr.dropFront(1);
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

        ++stateHeaders._conveyor._totalHeadersCount;
        _procesor = &InputSlicer::headerPreValue;
        return headerPreValue(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerPreValue(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::SourceAdapter::ForHdr& saForHdr = sa.forHdr();

        inputSlicer::state::Headers& stateHeaders = state<inputSlicer::state::Headers>();
        dbgAssert(inputSlicer::state::Headers::Current::Kind::regular == stateHeaders._current._kind ||
                  inputSlicer::state::Headers::Current::Kind::valueContinue == stateHeaders._current._kind);

        while(!saForHdr.empty())
        {
            if(isspace(saForHdr.front()))
                saForHdr.dropFront(1);
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

        _procesor = &InputSlicer::headerLF;
        return headerLF(sa);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::headerLF(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::SourceAdapter::ForHdr& saForHdr = sa.forHdr();

        inputSlicer::state::Headers& stateHeaders = state<inputSlicer::state::Headers>();
        dbgAssert(inputSlicer::state::Headers::Current::Kind::unknown != stateHeaders._current._kind);

        if(saForHdr.empty())
            return inputSlicer::Result::needMore;

        if('\n' != saForHdr.front())
            return inputSlicer::Result::badEntity;

        saForHdr.dropFront(1);

        inputSlicer::Result result{};
        switch(stateHeaders._current._kind)
        {
        case inputSlicer::state::Headers::Current::Kind::unknown:
            dbgAssert("unparsed header declated as sliced");
            return inputSlicer::Result::badEntity;

        case inputSlicer::state::Headers::Current::Kind::regular:
            {
                if(!stateHeaders._current._key._size)
                    return inputSlicer::Result::badEntity;

                for(std::size_t i{}; i<stateHeaders._current._key._size; ++i)
                {
                    char c = stateHeaders._current._key._downstream[i];
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

                rtrim(stateHeaders._current._value._downstream);
                stateHeaders._conveyor._totalValueSize += stateHeaders._current._value._downstream.size();
                if(inputSlicer::state::_maxEntityHeaderValueSize <= stateHeaders._conveyor._totalValueSize)
                    return inputSlicer::Result::tooBigHeaders;

                std::optional<api::http::header::KeyRecognized> keyRecognized = enumSupport::toEnum<api::http::header::KeyRecognized>(stateHeaders._current._key.str());
                if(keyRecognized)
                {
                    auto split = [](std::string_view str, std::string_view delims, auto f)
                    {
                        std::string_view::size_type pos{};
                        for(;;)
                        {
                            std::string_view::size_type next = str.find_first_of(delims, pos);
                            std::string_view sub = str.substr(pos, next-pos);
                            if(!sub.empty())
                                f(sub);
                            if(std::string_view::npos == next)
                                break;
                            pos = next+1;
                        }
                    };

                    auto setCompression = [&](std::string_view type) -> bool
                    {
                        if(inputSlicer::state::Headers::BodyRelated::Compression::none != stateHeaders._bodyRelated._compression)
                            return false;

                        if("compress"sv == type)
                            return false;
                        else if("deflate"sv == type)
                            stateHeaders._bodyRelated._compression = inputSlicer::state::Headers::BodyRelated::Compression::deflate;
                        else if("gzip"sv == type)
                            stateHeaders._bodyRelated._compression = inputSlicer::state::Headers::BodyRelated::Compression::gzip;
                        else if("br"sv == type)
                            stateHeaders._bodyRelated._compression = inputSlicer::state::Headers::BodyRelated::Compression::br;
                        else if("zstd"sv == type)
                            stateHeaders._bodyRelated._compression = inputSlicer::state::Headers::BodyRelated::Compression::zstd;
                        else
                            return false;

                        return true;
                    };

                    switch(*keyRecognized)
                    {
                    case api::http::header::KeyRecognized::Content_Length:
                        stateHeaders._conveyor._allowLastValueContinue = false;

                        if( inputSlicer::state::Headers::BodyRelated::Portionality::null == stateHeaders._bodyRelated._portionality ||
                            inputSlicer::state::Headers::BodyRelated::Portionality::untilClose == stateHeaders._bodyRelated._portionality)
                        {
                            stateHeaders._bodyRelated._portionality = inputSlicer::state::Headers::BodyRelated::Portionality::byContentLength;
                        }
                        else
                            return inputSlicer::Result::badEntity;

                        {
                            const char* txtBegin = stateHeaders._current._value._downstream.data();
                            const char* txtEnd = txtBegin + stateHeaders._current._value._downstream.size();
                            auto [prsEnd, ec] = std::from_chars(txtBegin, txtEnd, stateHeaders._bodyRelated._contentLength);
                            if(ec != std::errc{} || prsEnd != txtEnd)
                                return inputSlicer::Result::badEntity;
                        }
                        break;
                    case api::http::header::KeyRecognized::Content_Encoding:
                        stateHeaders._conveyor._allowLastValueContinue = false;
                        {
                            bool someBadValue = false;
                            split(stateHeaders._current._value._downstream, ", "sv, [&](std::string_view part)
                            {
                                someBadValue |= !setCompression(part);
                            });

                            if(someBadValue)
                                return inputSlicer::Result::unprocessableContent;
                        }
                        break;
                    case api::http::header::KeyRecognized::Transfer_Encoding:
                        stateHeaders._conveyor._allowLastValueContinue = false;
                        {
                            bool someBadValue = false;
                            split(stateHeaders._current._value._downstream, ", "sv, [&](std::string_view part)
                            {
                                if("chunked"sv == part)
                                {
                                    if( inputSlicer::state::Headers::BodyRelated::Portionality::null == stateHeaders._bodyRelated._portionality ||
                                        inputSlicer::state::Headers::BodyRelated::Portionality::untilClose == stateHeaders._bodyRelated._portionality)
                                    {
                                        stateHeaders._bodyRelated._portionality = inputSlicer::state::Headers::BodyRelated::Portionality::chunked;
                                    }
                                    else
                                        someBadValue = true;
                                }
                                else
                                    someBadValue |= !setCompression(part);
                            });

                            if(someBadValue)
                                return inputSlicer::Result::unprocessableContent;
                        }
                        break;
                    case api::http::header::KeyRecognized::Connection:
                        stateHeaders._conveyor._allowLastValueContinue = false;
                        if("close"sv == stateHeaders._current._value._downstream)
                        {
                            if(inputSlicer::state::Headers::BodyRelated::Portionality::null == stateHeaders._bodyRelated._portionality)
                                stateHeaders._bodyRelated._portionality = inputSlicer::state::Headers::BodyRelated::Portionality::untilClose;
                        }
                        break;
                    case api::http::header::KeyRecognized::Trailer:
                        stateHeaders._conveyor._allowLastValueContinue = false;
                        split(stateHeaders._current._value._downstream, ", "sv, [&](std::string_view part)
                        {
                            stateHeaders._bodyRelated._trailers.emplace(part);
                        });
                        break;
                    default:
                        stateHeaders._conveyor._allowLastValueContinue = true;
                        break;
                    }

                    stateHeaders._conveyor._tail.emplace_back(*keyRecognized, std::move(stateHeaders._current._value._downstream));
                }
                else
                {
                    stateHeaders._conveyor._allowLastValueContinue = true;
                    stateHeaders._conveyor._tail.emplace_back(api::http::header::KeyAny{stateHeaders._current._key.str()}, std::move(stateHeaders._current._value._downstream));
                }
            }
            result = inputSlicer::Result::needMore;
            break;

        case inputSlicer::state::Headers::Current::Kind::valueContinue:
            {
                dbgAssert(stateHeaders._current._key.empty());
                if(!stateHeaders._conveyor._allowLastValueContinue || stateHeaders._conveyor._tail.empty())
                    return inputSlicer::Result::badEntity;

                std::string& value = stateHeaders._conveyor._tail.back().value;
                std::string addition = trim(std::move(stateHeaders._current._value._downstream));

                std::size_t totalValueSize = value.size() + 1 + addition.size();
                if(totalValueSize > decltype(stateHeaders._current._value)::_limit)
                    return inputSlicer::Result::tooBigHeaders;

                value.reserve(totalValueSize);
                value += ' ';
                value += std::move(addition);
            }
            result = inputSlicer::Result::needMore;
            break;

        case inputSlicer::state::Headers::Current::Kind::empty:
            result = inputSlicer::Result::done;
            break;
        }

        switch(result)
        {
        case inputSlicer::Result::needMore:
            if(saForHdr.empty())
            {
                result = static_cast<Derived*>(this)->sliceFlush(stateHeaders, false);
                if(inputSlicer::Result::needMore != result)
                    return result;
            }
            stateHeaders._current.reset();
            _procesor = &InputSlicer::headerPreKey;
            return headerPreKey(sa);

        case inputSlicer::Result::done:
            result = static_cast<Derived*>(this)->sliceFlush(stateHeaders, true);
            if(inputSlicer::Result::done != result)
                return result;

            {
                auto bodySetup = [compression = stateHeaders._bodyRelated._compression, trailers = std::move(stateHeaders._bodyRelated._trailers)](inputSlicer::state::Body& stateBody)
                {
                    stateBody._trailers = std::move(trailers);
                    switch(compression)
                    {
                    case inputSlicer::state::Headers::BodyRelated::Compression::none:
                        return stateBody._decompressor.emplace<compress::None>().initialize();
                    case inputSlicer::state::Headers::BodyRelated::Compression::deflate:
                        return stateBody._decompressor.emplace<compress::Zlib<compress::zlib::Type::deflate, compress::Direction::decompress>>().initialize();
                    case inputSlicer::state::Headers::BodyRelated::Compression::gzip:
                        return stateBody._decompressor.emplace<compress::Zlib<compress::zlib::Type::gzip, compress::Direction::decompress>>().initialize();
                    case inputSlicer::state::Headers::BodyRelated::Compression::br:
                        return stateBody._decompressor.emplace<compress::Br<compress::Direction::decompress>>().initialize();
                    case inputSlicer::state::Headers::BodyRelated::Compression::zstd:
                        return stateBody._decompressor.emplace<compress::Zstd<compress::Direction::decompress>>().initialize();
                    }
                    return false;
                };

                switch(stateHeaders._bodyRelated._portionality)
                {
                case inputSlicer::state::Headers::BodyRelated::Portionality::null:
                case inputSlicer::state::Headers::BodyRelated::Portionality::untilClose:
                    {
                        static_cast<Derived*>(this)->_emitDataDoneOnClose = true;

                        if(!bodySetup(state<inputSlicer::state::BodyUntilClose, false>()))
                            return inputSlicer::Result::internalError;
                        _procesor = &InputSlicer::bodyUntilClose;
                        return bodyUntilClose(sa);
                    }
                case inputSlicer::state::Headers::BodyRelated::Portionality::byContentLength:
                    {
                        auto contentLength = stateHeaders._bodyRelated._contentLength;
                        inputSlicer::state::BodyByContentLength& bodyState = state<inputSlicer::state::BodyByContentLength, false>();
                        bodyState._contentLength = contentLength;
                        if(!bodySetup(bodyState))
                            return inputSlicer::Result::internalError;
                        _procesor = &InputSlicer::bodyByContentLength;
                        return bodyByContentLength(sa);
                    }
                case inputSlicer::state::Headers::BodyRelated::Portionality::chunked:
                    {
                        if(!bodySetup(state<inputSlicer::state::BodyChunked, false>()))
                            return inputSlicer::Result::internalError;
                        _procesor = &InputSlicer::bodyChunked;
                        return bodyChunked(sa);
                    }
                }

                unreacheable();
            }

        default:
            unreacheable();
        }

        unreacheable();
        return result;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::bodyUntilClose(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::state::BodyUntilClose& stateBody = state<inputSlicer::state::BodyUntilClose>();

        if(stateBody.needDecompress())
        {
            std::optional<Bytes> decompressed = stateBody.decompress(sa.forBody().detach(), false);
            if(!decompressed)
                return inputSlicer::Result::badEntity;
            stateBody._content.end().write(std::move(*decompressed));
        }
        else
            stateBody._content.end().write(sa.forBody().detach());

        if(!stateBody._content.empty())
            return static_cast<Derived*>(this)->sliceFlush(stateBody, false);

        return inputSlicer::Result::needMore;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::bodyByContentLength(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::state::BodyByContentLength& stateBody = state<inputSlicer::state::BodyByContentLength>();

        Bytes content = sa.forBody().detach(stateBody._contentLength);
        dbgAssert(content.size() <= stateBody._contentLength);
        stateBody._contentLength -= content.size();
        bool done = !stateBody._contentLength;

        if(stateBody.needDecompress())
        {
            std::optional<Bytes> decompressed = stateBody.decompress(std::move(content), done);
            if(!decompressed)
                return inputSlicer::Result::badEntity;
            stateBody._content.end().write(std::move(*decompressed));
        }
        else
            stateBody._content.end().write(std::move(content));

        if(!stateBody._content.empty() || done)
            return static_cast<Derived*>(this)->sliceFlush(stateBody, done);

        return done ? inputSlicer::Result::done : inputSlicer::Result::needMore;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    inputSlicer::Result InputSlicer<mode, Derived>::bodyChunked(inputSlicer::SourceAdapter& sa)
    {
        inputSlicer::state::BodyChunked& stateBody = state<inputSlicer::state::BodyChunked>();

        auto flush = [&](inputSlicer::Result res)
        {
            bool done = inputSlicer::Result::done == res;

            if(done && stateBody.needDecompress())
            {
                std::optional<Bytes> decompressed = stateBody.decompress(Bytes{}, true);
                if(!decompressed)
                    return inputSlicer::Result::badEntity;
                stateBody._content.end().write(std::move(*decompressed));
            }

            if(!stateBody._content.empty() || done)
                return static_cast<Derived*>(this)->sliceFlush(stateBody, done);
            return res;
        };

        for(;;)
        {
            switch(stateBody._state)
            {
            case inputSlicer::state::BodyChunked::State::null:
                stateBody._length = 0;
                stateBody._state = inputSlicer::state::BodyChunked::State::length;
                break;
            case inputSlicer::state::BodyChunked::State::length:
                {
                    inputSlicer::SourceAdapter::ForHdr& saForHdr = sa.forHdr();
                    for(;;)
                    {
                        if(saForHdr.empty())
                            return flush(inputSlicer::Result::needMore);

                        char c = saForHdr.front();
                        if('\r' == c)
                        {
                            saForHdr.dropFront(1);
                            stateBody._state = inputSlicer::state::BodyChunked::State::LF0;
                            break;
                        }

                        if(0x10000000 <= stateBody._length)
                            return inputSlicer::Result::tooBigContent;

                        uint32 digit{};
                        if('0' <= c && '9' >= c)
                            digit = c - '0';
                        else if('a' <= c && 'f' >= c)
                            digit = c + 10 - 'a';
                        else if('A' <= c && 'F' >= c)
                            digit = c + 10 - 'A';
                        else
                            return inputSlicer::Result::badEntity;

                        saForHdr.dropFront(1);
                        stateBody._length <<= 4;
                        stateBody._length |= digit;
                    }
                }
                break;
            case inputSlicer::state::BodyChunked::State::LF0:
                {
                    inputSlicer::SourceAdapter::ForHdr& saForHdr = sa.forHdr();
                    if(saForHdr.empty())
                        return flush(inputSlicer::Result::needMore);
                    if('\n' != saForHdr.front())
                        return inputSlicer::Result::badEntity;
                    saForHdr.dropFront(1);
                    stateBody._state = stateBody._length ?
                                           inputSlicer::state::BodyChunked::State::content :
                                           inputSlicer::state::BodyChunked::State::done;
                }
                break;
            case inputSlicer::state::BodyChunked::State::content:
                {
                    Bytes content = sa.forBody().detach(stateBody._length);
                    dbgAssert(content.size() <= stateBody._length);
                    stateBody._length -= content.size();

                    if(stateBody.needDecompress())
                    {
                        std::optional<Bytes> decompressed = stateBody.decompress(std::move(content), false);
                        if(!decompressed)
                            return inputSlicer::Result::badEntity;
                        stateBody._content.end().write(std::move(*decompressed));
                    }
                    else
                        stateBody._content.end().write(std::move(content));

                    if(stateBody._length)
                    {
                        dbgAssert(sa.forBody().empty());
                        return flush(inputSlicer::Result::needMore);
                    }

                    stateBody._state = inputSlicer::state::BodyChunked::State::CR1;
                }
                break;
            case inputSlicer::state::BodyChunked::State::CR1:
                {
                    inputSlicer::SourceAdapter::ForHdr& saForHdr = sa.forHdr();
                    if(saForHdr.empty())
                        return flush(inputSlicer::Result::needMore);
                    if('\r' != saForHdr.front())
                        return inputSlicer::Result::badEntity;
                    saForHdr.dropFront(1);
                    stateBody._state = inputSlicer::state::BodyChunked::State::LF1;
                }
                break;
            case inputSlicer::state::BodyChunked::State::LF1:
                {
                    inputSlicer::SourceAdapter::ForHdr& saForHdr = sa.forHdr();
                    if(saForHdr.empty())
                        return flush(inputSlicer::Result::needMore);
                    if('\n' != saForHdr.front())
                        return inputSlicer::Result::badEntity;
                    saForHdr.dropFront(1);
                    stateBody._state = inputSlicer::state::BodyChunked::State::null;
                }
                break;
            case inputSlicer::state::BodyChunked::State::done:
                return flush(inputSlicer::Result::done);
            }
        }

        unreacheable();
        return inputSlicer::Result::badEntity;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <inputSlicer::Mode mode, class Derived>
    template <class S, bool optimistic>
    S& InputSlicer<mode, Derived>::state()
    {
        if constexpr(optimistic)
        {
            if constexpr(std::is_same_v<S, inputSlicer::state::RequestNull>)        {dbgAssert(ActiveState::requestNull         == _activeState); return _requestNull;        }
            if constexpr(std::is_same_v<S, inputSlicer::state::RequestFirstLine>)   {dbgAssert(ActiveState::requestFirstLine    == _activeState); return _requestFirstLine;   }
            if constexpr(std::is_same_v<S, inputSlicer::state::ResponseNull>)       {dbgAssert(ActiveState::responseNull        == _activeState); return _responseNull;       }
            if constexpr(std::is_same_v<S, inputSlicer::state::ResponseFirstLine>)  {dbgAssert(ActiveState::responseFirstLine   == _activeState); return _responseFirstLine;  }
            if constexpr(std::is_same_v<S, inputSlicer::state::Headers>)            {dbgAssert(ActiveState::headers             == _activeState); return _headers;            }
            if constexpr(std::is_same_v<S, inputSlicer::state::BodyUntilClose>)     {dbgAssert(ActiveState::bodyUntilClose      == _activeState); return _bodyUntilClose;     }
            if constexpr(std::is_same_v<S, inputSlicer::state::BodyByContentLength>){dbgAssert(ActiveState::bodyByContentLength == _activeState); return _bodyByContentLength;}
            if constexpr(std::is_same_v<S, inputSlicer::state::BodyChunked>)        {dbgAssert(ActiveState::bodyChunked         == _activeState); return _bodyChunked;        }
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
            case ActiveState::bodyUntilClose:
                if constexpr(std::is_same_v<S, inputSlicer::state::BodyUntilClose>)     return _bodyUntilClose;
                _bodyUntilClose.~BodyUntilClose();
                break;
            case ActiveState::bodyByContentLength:
                if constexpr(std::is_same_v<S, inputSlicer::state::BodyByContentLength>)return _bodyByContentLength;
                _bodyByContentLength.~BodyByContentLength();
                break;
            case ActiveState::bodyChunked:
                if constexpr(std::is_same_v<S, inputSlicer::state::BodyChunked>)        return _bodyChunked;
                _bodyChunked.~BodyChunked();
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

            if constexpr(std::is_same_v<S, inputSlicer::state::BodyUntilClose>)
            {
                _activeState = ActiveState::bodyUntilClose;
                return *(new (&_bodyUntilClose) S);
            }

            if constexpr(std::is_same_v<S, inputSlicer::state::BodyByContentLength>)
            {
                _activeState = ActiveState::bodyByContentLength;
                return *(new (&_bodyByContentLength) S);
            }

            if constexpr(std::is_same_v<S, inputSlicer::state::BodyChunked>)
            {
                _activeState = ActiveState::bodyChunked;
                return *(new (&_bodyChunked) S);
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
        if constexpr(std::is_same_v<S, inputSlicer::state::RequestNull>)        {dbgAssert(ActiveState::requestNull         == _activeState); return _requestNull;        }
        if constexpr(std::is_same_v<S, inputSlicer::state::RequestFirstLine>)   {dbgAssert(ActiveState::requestFirstLine    == _activeState); return _requestFirstLine;   }
        if constexpr(std::is_same_v<S, inputSlicer::state::ResponseNull>)       {dbgAssert(ActiveState::responseNull        == _activeState); return _responseNull;       }
        if constexpr(std::is_same_v<S, inputSlicer::state::ResponseFirstLine>)  {dbgAssert(ActiveState::responseFirstLine   == _activeState); return _responseFirstLine;  }
        if constexpr(std::is_same_v<S, inputSlicer::state::Headers>)            {dbgAssert(ActiveState::headers             == _activeState); return _headers;            }
        if constexpr(std::is_same_v<S, inputSlicer::state::BodyUntilClose>)     {dbgAssert(ActiveState::bodyUntilClose      == _activeState); return _bodyUntilClose;     }
        if constexpr(std::is_same_v<S, inputSlicer::state::BodyByContentLength>){dbgAssert(ActiveState::bodyByContentLength == _activeState); return _bodyByContentLength;}
        if constexpr(std::is_same_v<S, inputSlicer::state::BodyChunked>)        {dbgAssert(ActiveState::bodyChunked         == _activeState); return _bodyChunked;        }

        unreacheable();
        static S s;
        return s;
    }
}
