/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "br.hpp"
#include "ioRoller.hpp"

namespace dci::module::www::http::compress
{
    using namespace std::string_view_literals;

    namespace
    {
        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        struct BufIn
        {
            const uint8_t*  _ptr;
            size_t          _size;
        };

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        struct BufOut
        {
            uint8_t*    _ptr;
            size_t      _size;
        };
    }

    namespace
    {
        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <Direction direction> struct Algo;

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <> struct Algo<Direction::compress>
        {
            static std::string_view name(){ return "br"sv; }
            static BrotliEncoderState* alloc()
            {
                return BrotliEncoderCreateInstance(
                    [](void* /*opaque*/, size_t size) -> void* { return mm::heap::alloc(size); },
                    [](void* /*opaque*/, void* ptr) -> void { return mm::heap::free(ptr); },
                    nullptr);
            }
            static void free(BrotliEncoderState* state){ return BrotliEncoderDestroyInstance(state); }
            static bool step(BrotliEncoderState* state, BufIn& in, BufOut& out, BrotliEncoderOperation op){ return BrotliEncoderCompressStream(state, op, &in._size, &in._ptr, &out._size, &out._ptr, nullptr); }
            static bool finished(BrotliEncoderState *state){ return BrotliEncoderIsFinished(state); }
        };

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <> struct Algo<Direction::decompress>
        {
            static std::string_view name(){ return "unbr"sv; }
            static BrotliDecoderState* alloc()
            {
                return BrotliDecoderCreateInstance(
                    [](void* /*opaque*/, size_t size) -> void* { return mm::heap::alloc(size); },
                    [](void* /*opaque*/, void* ptr) -> void { return mm::heap::free(ptr); },
                    nullptr);
            }
            static void free(BrotliDecoderState* state){ return BrotliDecoderDestroyInstance(state); }
            static BrotliDecoderResult step(BrotliDecoderState* state, BufIn& in, BufOut& out){ return BrotliDecoderDecompressStream(state, &in._size, &in._ptr, &out._size, &out._ptr, nullptr); }
            static const char* error(BrotliDecoderState* state){ return BrotliDecoderErrorString(BrotliDecoderGetErrorCode(state)); }
            static bool finished(BrotliDecoderState *state){ return BrotliDecoderIsFinished(state); }
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <Direction direction>
    Br<direction>::~Br()
    {
        if(_state)
            Algo<direction>::free(_state);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <Direction direction>
    bool Br<direction>::initialize()
    {
        _state = Algo<direction>::alloc();
        if(!_state)
        {
            LOGD(Algo<direction>::name() << " alloc failed");
            return false;
        }
        return true;
    }

    namespace
    {
        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        struct BrIoRoller : IoRoller<BrIoRoller>
        {
            BrIoRoller(Bytes&& src, Bytes& dst)
                : IoRoller<BrIoRoller>{std::move(src), dst}
            {
            }

            void setIn(const void* ptr, uint32 size)
            {
                _in._ptr = static_cast<const uint8_t*>(ptr);
                _in._size = size;
            }

            uint32 getInSize() const
            {
                return _in._size;
            }

            void setOut(void* ptr, uint32 size)
            {
                _out._ptr = static_cast<uint8_t*>(ptr);;
                _out._size = size;
            }

            uint32 getOutSize() const
            {
                return _out._size;
            }

        public:
            BufIn   _in{};
            BufOut  _out{};
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <Direction direction>
    std::optional<Bytes> Br<direction>::exec(Bytes&& content, bool finish)
    {
        if(!_state)
            return {};

        Bytes res;
        {
            BrIoRoller ioRoller{std::move(content), res};

            for(;;)
            {
                ioRoller.step();

                if constexpr(Direction::compress == direction)
                {
                    BrotliEncoderOperation op;
                    if(ioRoller.srcAtFinish())
                        op = finish ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_FLUSH;
                    else
                        op = BROTLI_OPERATION_PROCESS;

                    if(!Algo<direction>::step(_state, ioRoller._in, ioRoller._out, op))
                    {
                        LOGD(Algo<direction>::name() << " failed");
                        return {};
                    }
                }
                else
                {
                    BrotliDecoderResult stepRes = Algo<direction>::step(_state, ioRoller._in, ioRoller._out);
                    if(BROTLI_DECODER_RESULT_ERROR == stepRes)
                    {
                        LOGD(Algo<direction>::name() << " failed: " << Algo<direction>::error(_state));
                        return {};
                    }

                    if(BROTLI_DECODER_RESULT_SUCCESS == stepRes)
                        break;
                }
            }
        }

        bool dstFinished = Algo<direction>::finished(_state);

        if(finish && !dstFinished)
        {
            LOGD(Algo<direction>::name() << ": incomplete source");
            return {};
        }

        if(dstFinished && !content.empty())
        {
            LOGD(Algo<direction>::name() << ": extra source");
            return {};
        }

        return res;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template class Br<Direction::compress>;
    template class Br<Direction::decompress>;
}
