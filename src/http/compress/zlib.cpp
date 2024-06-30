/* This file is part of the the dci project. Copyright (C) 2013-2023 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include "pch.hpp"
#include "zlib.hpp"
#include "ioRoller.hpp"

namespace dci::module::www::http::compress
{
    using namespace std::string_view_literals;

    namespace
    {
        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <zlib::Type type, Direction direction> struct Algo;

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <> struct Algo<zlib::Type::deflate, Direction::compress>
        {
            static std::string_view name(){ return "deflate"sv; }
            static int init(z_stream* strm){ return deflateInit2(strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY); }
            static int end(z_stream* strm){ return deflateEnd(strm); }
            static int step(z_stream* strm, int flush){ return deflate(strm, flush); }
        };

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <> struct Algo<zlib::Type::deflate, Direction::decompress>
        {
            static std::string_view name(){ return "inflate"sv; }
            static int init(z_stream* strm){ return inflateInit2(strm, -MAX_WBITS); }
            static int end(z_stream* strm){ return inflateEnd(strm); }
            static int step(z_stream* strm, int flush){ return inflate(strm, flush); }
        };

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <> struct Algo<zlib::Type::gzip, Direction::compress>
        {
            static std::string_view name(){ return "gzip"sv; }
            static int init(z_stream* strm)
            {
                if(int i = deflateInit2(strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS+16 , MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY); Z_OK != i)
                    return i;
                gz_header hdr{};
                return deflateSetHeader(strm, &hdr);
            }
            static int end(z_stream* strm){ return deflateEnd(strm); }
            static int step(z_stream* strm, int flush){ return deflate(strm, flush); }
        };

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        template <> struct Algo<zlib::Type::gzip, Direction::decompress>
        {
            static std::string_view name(){ return "gunzip"sv; }
            static int init(z_stream* strm){ return inflateInit2(strm, MAX_WBITS+16); }
            static int end(z_stream* strm){ return inflateEnd(strm); }
            static int step(z_stream* strm, int flush){ return inflate(strm, flush); }
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <zlib::Type type, Direction direction>
    Zlib<type, direction>::~Zlib()
    {
        if(_initialized)
            Algo<type, direction>::end(&_strm);
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <zlib::Type type, Direction direction>
    bool Zlib<type, direction>::initialize()
    {
        dbgAssert(!_initialized);

        _strm.zalloc = [](voidpf /*opaque*/, unsigned items, unsigned size){ return mm::heap::alloc(items*size); };
        _strm.zfree = [](voidpf /*opaque*/, voidpf address){ return mm::heap::free(address); };
        //_strm.opaque = this;

        int i = Algo<type, direction>::init(&_strm);
        if(Z_OK != i)
        {
            LOGD(Algo<type, direction>::name() << ": init failed: " << zError(i));
            return _initialized;
        }

        dbgAssert(!_strm.msg);
        _initialized = true;
        return _initialized;
    }

    namespace
    {
        struct ZlibIoRollerStreamHolder
        {
            z_stream& _strm;
        };

        /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
        struct ZlibIoRoller
            : ZlibIoRollerStreamHolder
            , IoRoller<ZlibIoRoller>
        {
            ZlibIoRoller(z_stream& strm, Bytes&& src, Bytes& dst)
                : ZlibIoRollerStreamHolder{strm}
                , IoRoller<ZlibIoRoller>{std::move(src), dst}
            {
            }

            void setIn(const void* ptr, uint32 size)
            {
                _strm.next_in = const_cast<z_const Bytef *>(static_cast<const Bytef *>(ptr));
                _strm.avail_in = size;
            }

            uint32 getInSize() const
            {
                return _strm.avail_in;
            }

            void setOut(void* ptr, uint32 size)
            {
                _strm.next_out = static_cast<Bytef *>(ptr);
                _strm.avail_out = size;
            }

            uint32 getOutSize() const
            {
                return _strm.avail_out;
            }
        };
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template <zlib::Type type, Direction direction>
    std::optional<Bytes> Zlib<type, direction>::exec(Bytes&& content, bool finish)
    {
        if(!_initialized)
            return {};

        if(_strm.msg)
        {
            LOGD(Algo<type, direction>::name() << ": bad state: " << _strm.msg);
            return {};
        }

        if(_dstFinished)
        {
            LOGD(Algo<type, direction>::name() << ": extra source");
            return {};
        }

        Bytes res;
        {
            ZlibIoRoller ioRoller{_strm, std::move(content), res};

            for(;;)
            {
                ioRoller.step();

                int flushMode;
                if(ioRoller.srcAtFinish())
                    flushMode = finish ? Z_FINISH : Z_SYNC_FLUSH;
                else
                    flushMode = Z_NO_FLUSH;

                int i = Algo<type, direction>::step(&_strm, flushMode);

                switch(i)
                {
                case Z_OK:
                    continue;
                case Z_STREAM_END:
                    _dstFinished = true;
                    break;
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_STREAM_ERROR:
                case Z_MEM_ERROR:
                case Z_BUF_ERROR:
                default:
                    dbgAssert(_strm.msg);
                    if(_strm.msg)
                        LOGD(Algo<type, direction>::name() << " failed: " << zError(i) << ", " << _strm.msg);
                    else
                        LOGD(Algo<type, direction>::name() << " failed: " << zError(i));

                    _dstFinished = true;
                    return {};
                }

                if(_dstFinished)
                    break;
            }
        }

        if(!_dstFinished && finish)
        {
            LOGD(Algo<type, direction>::name() << ": incomplete source");
            return {};
        }

        if(_dstFinished && !content.empty())
        {
            LOGD(Algo<type, direction>::name() << ": extra source");
            return {};
        }

        return res;
    }

    /////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
    template class Zlib<zlib::Type::deflate, Direction::compress>;
    template class Zlib<zlib::Type::deflate, Direction::decompress>;
    template class Zlib<zlib::Type::gzip, Direction::compress>;
    template class Zlib<zlib::Type::gzip, Direction::decompress>;
}
