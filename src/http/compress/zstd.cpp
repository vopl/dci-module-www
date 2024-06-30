/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "zstd.hpp"
#include "ioRoller.hpp"

namespace dci::module::www::http::compress
{
    using namespace std::string_view_literals;

    namespace
    {
        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <Direction direction> struct Algo;

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <> struct Algo<Direction::compress>
        {
            static std::string_view name(){ return "zstd"sv; }
            static ZSTD_CCtx* alloc() { return ZSTD_createCCtx(); }
            static size_t free(ZSTD_CCtx* ctx){ return ZSTD_freeCCtx(ctx); }
            static size_t step(ZSTD_CCtx* ctx, ZSTD_outBuffer* out, ZSTD_inBuffer* in, ZSTD_EndDirective end){ return ZSTD_compressStream2(ctx, out, in, end); }
        };

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <> struct Algo<Direction::decompress>
        {
            static std::string_view name(){ return "unzstd"sv; }
            static ZSTD_DCtx* alloc(){ return ZSTD_createDCtx(); }
            static size_t free(ZSTD_DCtx* ctx){ return ZSTD_freeDCtx(ctx); }
            static size_t step(ZSTD_DCtx* ctx, ZSTD_outBuffer* out, ZSTD_inBuffer* in){ return ZSTD_decompressStream(ctx, out, in); }
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <Direction direction>
    Zstd<direction>::~Zstd()
    {
        if(_ctx)
            Algo<direction>::free(_ctx);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <Direction direction>
    bool Zstd<direction>::initialize()
    {
        _ctx = Algo<direction>::alloc();
        if(!_ctx)
        {
            LOGD(Algo<direction>::name() << " alloc failed");
            return false;
        }
        return true;
    }

    namespace
    {
        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        struct ZstdIoRoller : IoRoller<ZstdIoRoller>
        {
            ZstdIoRoller(Bytes&& src, Bytes& dst)
                : IoRoller<ZstdIoRoller>{std::move(src), dst}
            {
            }

            void setIn(const void* ptr, uint32 size)
            {
                _in.src = ptr;
                _in.size = size;
                _in.pos = 0;
            }

            uint32 getInSize() const
            {
                return _in.size - _in.pos;
            }

            void setOut(void* ptr, uint32 size)
            {
                _out.dst = ptr;
                _out.size = size;
                _out.pos = 0;
            }

            uint32 getOutSize() const
            {
                return _out.size - _out.pos;
            }

        public:
            ZSTD_inBuffer   _in{};
            ZSTD_outBuffer  _out{};
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <Direction direction>
    std::optional<Bytes> Zstd<direction>::exec(Bytes&& content, bool finish)
    {
        if(!_ctx)
            return {};

        bool dstUnflushed{};
        Bytes res;
        {
            ZstdIoRoller ioRoller{std::move(content), res};

            for(;;)
            {
                ioRoller.step();

                size_t stepRes;
                if constexpr(Direction::compress == direction)
                {
                    ZSTD_EndDirective end;
                    if(ioRoller.srcAtFinish())
                        end = finish ? ZSTD_e_end : ZSTD_e_flush;
                    else
                        end = ZSTD_e_continue;

                    stepRes = Algo<direction>::step(_ctx, &ioRoller._out, &ioRoller._in, end);
                }
                else
                    stepRes = Algo<direction>::step(_ctx, &ioRoller._out, &ioRoller._in);

                if(ZSTD_isError(stepRes))
                {
                    LOGD(Algo<direction>::name() << " failed: " << ZSTD_getErrorName(stepRes));
                    return {};
                }

                if(!ioRoller.getInSize() && ioRoller.getOutSize()) // пусто на входе и есть место на выходе - это отсутствие прогресса
                {
                    dstUnflushed = !!stepRes;
                    break;
                }
            }
        }

        if(finish && dstUnflushed)
        {
            LOGD(Algo<direction>::name() << ": incomplete source");
            return {};
        }

        return res;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template class Zstd<Direction::compress>;
    template class Zstd<Direction::decompress>;
}
