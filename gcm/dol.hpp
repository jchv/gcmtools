/*
 * dol.hpp - (C) 2012-2013 jchadwick <johnwchadwick@gmail.com>
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
 * Special note: technically speaking, there is no reason why a gamecube disk
 * must use .dol. The apploader is responsible for mapping and initializing
 * the executable in memory. We assume the file is dol to make our lives
 * easier (since official disks do use .dol.)
 */

struct dol {
    static const unsigned max_sections = 18;

    struct Section {
        uint32_t offset;
        uint32_t baseaddr;
        uint32_t size;

        nall::vector<uint8_t> buffer;
    };

    Section section[max_sections];

    uint32_t bssAddr, bssSize, entrypoint;

    inline bool read(nall::stream *strm);
    inline bool write(nall::stream *strm);

protected:
    inline static void fillto(nall::stream *strm, int offset);

    unsigned fstoffset;
    unsigned strtableoffset;
    unsigned currentry;
};

bool dol::read(nall::stream *strm) {
    unsigned i = 0, doloffset = strm->offset();

    for(i = 0; i < max_sections; ++i)
        section[i].offset = strm->readm(4);
    for(i = 0; i < max_sections; ++i)
        section[i].baseaddr = strm->readm(4);
    for(i = 0; i < max_sections; ++i)
        section[i].size = strm->readm(4);

    bssAddr = strm->readm(4);
    bssSize = strm->readm(4);
    entrypoint = strm->readm(4);

    for(i = 0; i < max_sections; ++i) {
        if(section[i].offset > 0 && section[i].size > 0) {
            section[i].buffer.reserve(section[i].size);
            strm->seek(doloffset + section[i].offset);
            strm->read(section[i].buffer.data(), section[i].size);
        }
    }

    return true;
}

bool dol::write(nall::stream *strm) {
    unsigned i = 0, doloffset = strm->offset(), endoffset = 0;

    for(i = 0; i < max_sections; ++i)
        strm->writem(section[i].offset, 4);
    for(i = 0; i < max_sections; ++i)
        strm->writem(section[i].baseaddr, 4);
    for(i = 0; i < max_sections; ++i)
        strm->writem(section[i].size, 4);

    strm->writem(bssAddr, 4);
    strm->writem(bssSize, 4);
    strm->writem(entrypoint, 4);

    for(i = 0; i < max_sections; ++i) {
        if(section[i].offset > 0 && section[i].size > 0) {
            fillto(strm, doloffset + section[i].offset);
            strm->write(section[i].buffer.data(), section[i].size);

            unsigned end = doloffset + section[i].offset + section[i].size;
            if(end > endoffset)
                endoffset = end;
        }

        strm->seek(endoffset);
    }

    return true;
}

void dol::fillto(nall::stream *strm, int offset)
{
    int position = strm->offset();

    while(position < offset) {
        strm->writem(0, 1);
        position++;
    }

    if(position > offset)
        strm->seek(offset);
}
