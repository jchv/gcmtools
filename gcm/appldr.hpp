/*
 * appldr.hpp - (C) 2012-2013 jchadwick <johnwchadwick@gmail.com>
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
 */

struct apploader {
    struct Header {
        uint8_t date[16];
        uint32_t entrypoint;
        uint32_t length;
        uint32_t trailer;
        uint8_t padding[4];
    } header;

    uint8_t *data;
    int size;

    inline bool read(nall::stream *strm);
    inline bool write(nall::stream *strm);

    inline apploader();
    inline ~apploader();

protected:
    inline uint32_t realsize();
};

bool apploader::read(nall::stream *strm) {
    if(!strm->readable())
        return false;

    strm->read(header.date, sizeof header.date);
    header.entrypoint = strm->readm(sizeof header.entrypoint);
    header.length     = strm->readm(sizeof header.length    );
    header.trailer    = strm->readm(sizeof header.trailer   );
    strm->read(header.padding, sizeof header.padding);

    size = ((header.length + header.trailer - 1) / 32) * 32;

    if(data) delete[] data;
    data = new uint8_t[size];

    strm->read(data, size);
}

bool apploader::write(nall::stream *strm) {
    if(!strm->writable())
        return false;

    strm->write (header.date      , sizeof header.date      );
    strm->writem(header.entrypoint, sizeof header.entrypoint);
    strm->writem(header.length    , sizeof header.length    );
    strm->writem(header.trailer   , sizeof header.trailer   );
    strm->write (header.padding   , sizeof header.padding   );

    size = ((header.length + header.trailer - 1) / 32) * 32;
    strm->write(data, size);
}

apploader::apploader() {
    data = 0;
}

apploader::~apploader() {
    if(data) delete[] data;
}
