/*
 * gcm.hpp - (C) 2012 nmn/john w. chadwick<johnwchadwick@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * --
 *
 * Because of the sheer size of a gamecube iso (~1.4 GiB max,) it would be
 * ridiculous to require storing/reading the entire disk into memory;
 * Therefore, this code works off of nall::stream.
 *
 * The stream is kept until either a. destruction, or b. gcm::close.
 *
 * It can be used pretty efficiently with mmap/memory streams, assuming
 * there's address space to spare.
 *
 * I'm also keeping the junk data in the headers, in case they have any
 * affect on things...
 */

#include <nall/stream.hpp>
#include <nall/stdint.hpp>
#include <nall/endian.hpp>
#include <nall/string.hpp>
#include <nall/vector.hpp>

namespace gamecube {

#include "gcm/appldr.hpp"
#include "gcm/fst.hpp"
#include "gcm/dol.hpp"

struct gcm {
    struct Header {
        uint8_t gameCode[4];
        uint8_t developerId[2];
        uint8_t diskId;
        uint8_t version;
        uint8_t audioStrm;
        uint8_t strmBufferLen;
        uint8_t reserved1[0x12];
        uint8_t magicWord[4]; // should be { 0xc2, 0x33, 0x9f, 0x3d }
        uint8_t gameName[0x3e0];
        uint32_t dbgMonOffset;
        uint32_t dbgMonBaseAddr;
        uint8_t reserved2[0x18];
        uint32_t dolOffset;
        uint32_t fstOffset;
        uint32_t fstSize;
        uint32_t fstSizeMax;
        uint32_t unknown1;
        uint32_t unknown2;
        uint32_t unknown3;
        uint32_t reserved3;
    } header;

    struct Info
    {
        uint32_t dbgMonSize;
        uint32_t simulatedMemSize;
        uint32_t argOffset;
        uint32_t dbgFlag;
        uint32_t trackLocation;
        uint32_t trackSize;
        uint32_t countryCode;
        uint32_t unknown1;
        uint8_t unknown2[0x1FE0];
    } info;

    apploader appldr;
    dol binary;
    fst filesystem;

    inline bool open(nall::stream *s);
    inline bool readBootHeader(nall::stream *s);
    inline bool readBi2Header(nall::stream *s);
    inline bool write(nall::stream *os);
    inline bool writeBootHeader(nall::stream *os);
    inline bool writeBi2Header(nall::stream *os);
    inline void close();

    inline gcm();
    inline ~gcm();

protected:
    nall::stream *strm;
};

bool gcm::open(nall::stream *s) {
    if(strm)
        delete strm;

    if(!s->readable() || !s->seekable())
        return false;

    // read header
    s->seek(0);
    readBootHeader(s);
    s->seek(0x440);
    readBi2Header(s);

    // read apploader
    s->seek(0x2440);
    appldr.read(s);

    // read dol
    s->seek(header.dolOffset);
    binary.read(s);

    // read fst
    s->seek(header.fstOffset);
    filesystem.read(s);

    strm = s;

    return true;
}

bool gcm::readBootHeader(nall::stream *s) {
    s->read(header.gameCode   , sizeof header.gameCode   );
    s->read(header.developerId, sizeof header.developerId);

    header.diskId         = s->read();
    header.version        = s->read();
    header.audioStrm      = s->read();
    header.strmBufferLen  = s->read();

    s->read(header.reserved1, sizeof header.reserved1);
    s->read(header.magicWord, sizeof header.magicWord);
    s->read(header.gameName , sizeof header.gameName );

    header.dbgMonOffset   = s->readm(sizeof header.dbgMonOffset  );
    header.dbgMonBaseAddr = s->readm(sizeof header.dbgMonBaseAddr);

    s->read(header.reserved2, sizeof header.reserved2);

    header.dolOffset      = s->readm(sizeof header.dolOffset     );
    header.fstOffset      = s->readm(sizeof header.fstOffset     );
    header.fstSize        = s->readm(sizeof header.fstSize       );
    header.fstSizeMax     = s->readm(sizeof header.fstSizeMax    );
    header.unknown1       = s->readm(sizeof header.unknown1      );
    header.unknown2       = s->readm(sizeof header.unknown2      );
    header.unknown3       = s->readm(sizeof header.unknown3      );
    header.reserved3      = s->readm(sizeof header.reserved3     );

    return true;
}

bool gcm::readBi2Header(nall::stream *s) {
    info.dbgMonSize       = s->readm(sizeof info.dbgMonSize      );
    info.simulatedMemSize = s->readm(sizeof info.simulatedMemSize);
    info.argOffset        = s->readm(sizeof info.argOffset       );
    info.dbgFlag          = s->readm(sizeof info.dbgFlag         );
    info.trackLocation    = s->readm(sizeof info.trackLocation   );
    info.trackSize        = s->readm(sizeof info.trackSize       );
    info.countryCode      = s->readm(sizeof info.countryCode     );
    info.unknown1         = s->readm(sizeof info.unknown1        );
    s->read(info.unknown2, sizeof info.unknown2);

    return true;
}

bool gcm::write(nall::stream *os) {
    if(!os->writable())
        return false;

    os->seek(0);
    writeBootHeader(os);
    os->seek(0x440);
    writeBi2Header(os);

    os->seek(0x2440);
    appldr.write(os);

    os->seek(header.dolOffset);
    binary.write(os);

    // If we need to, we can move the FST down.
    if(os->offset() > header.fstOffset)
        header.fstOffset = (os->offset() + 0xFFF) & -0x1000;

    os->seek(header.fstOffset);
    filesystem.write(os);

    return true;
}

inline bool gcm::writeBootHeader(nall::stream *os) {
    os->write(header.gameCode   , sizeof header.gameCode   );
    os->write(header.developerId, sizeof header.developerId);

    os->writem(header.diskId       , 1);
    os->writem(header.version      , 1);
    os->writem(header.audioStrm    , 1);
    os->writem(header.strmBufferLen, 1);

    os->write(header.reserved1, sizeof header.reserved1);
    os->write(header.magicWord, sizeof header.magicWord);
    os->write(header.gameName , sizeof header.gameName );

    os->writem(header.dbgMonOffset  , sizeof header.dbgMonOffset  );
    os->writem(header.dbgMonBaseAddr, sizeof header.dbgMonBaseAddr);

    os->write(header.reserved2, sizeof header.reserved2);

    os->writem(header.dolOffset , sizeof header.dolOffset );
    os->writem(header.fstOffset , sizeof header.fstOffset );
    os->writem(header.fstSize   , sizeof header.fstSize   );
    os->writem(header.fstSizeMax, sizeof header.fstSizeMax);
    os->writem(header.unknown1  , sizeof header.unknown1  );
    os->writem(header.unknown2  , sizeof header.unknown2  );
    os->writem(header.unknown3  , sizeof header.unknown3  );
    os->writem(header.reserved3 , sizeof header.reserved3 );
}

inline bool gcm::writeBi2Header(nall::stream *os) {
    os->writem(info.dbgMonSize      , sizeof info.dbgMonSize      );
    os->writem(info.simulatedMemSize, sizeof info.simulatedMemSize);
    os->writem(info.argOffset       , sizeof info.argOffset       );
    os->writem(info.dbgFlag         , sizeof info.dbgFlag         );
    os->writem(info.trackLocation   , sizeof info.trackLocation   );
    os->writem(info.trackSize       , sizeof info.trackSize       );
    os->writem(info.countryCode     , sizeof info.countryCode     );
    os->writem(info.unknown1        , sizeof info.unknown1        );
    os->write(info.unknown2, sizeof info.unknown2);
}

void gcm::close() {
    if(strm)
        delete strm;

    strm = 0;
    filesystem = fst();
    binary = dol();
    appldr = apploader();
}

gcm::gcm() {
    strm = 0;
}

gcm::~gcm() {
    if(strm) delete strm;
}

};
