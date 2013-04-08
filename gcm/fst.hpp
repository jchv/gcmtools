/*
 * fst.hpp - (C) 2012-2013 jchadwick <johnwchadwick@gmail.com>
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

static uint8_t *memdup(const uint8_t *data, unsigned len) {
    uint8_t *copy = new uint8_t[len];
    memcpy(copy, data, len);
    return copy;
}

enum reftype {
    none,
    bufref,
    streamref,
    fileref
};

struct fst {
    struct dataref {
        dataref()
        : buffer(0), strm(0), off(0), len(0), type(none) { }
        
        dataref(uint8_t *b, unsigned o, unsigned l)
        : buffer(b), strm(0), off(o), len(l), type(bufref) { }
        
        dataref(nall::stream *s, unsigned o, unsigned l)
        : buffer(0), strm(s), off(o), len(l), type(streamref) { }
        
        dataref(nall::string fn, unsigned o = 0, unsigned l = 0)
        : buffer(0), strm(0), filename(fn), off(o), len(l), type(fileref) {
            if(o == 0 && l == 0) {
                nall::file f;
                f.open(fn, nall::file::mode::read);
                len = f.size();
            }
        }
        ~dataref() {
            switch(type) {
            case bufref:
                delete buffer;
                buffer = 0;
                break;
            default:
                break;
            }
        }

        bool read(uint8_t *into) {
            switch(type) {
            case bufref:
                if(!buffer) return false;
                memcpy(into, buffer + off, len);
                break;
            case streamref:
                if(!strm) return false;
                unsigned oldoffset;
                oldoffset = strm->offset();
                strm->seek(off);
                strm->read(into, len);
                strm->seek(oldoffset);
                break;
            case fileref:
            {
                nall::file f;
                if(!f.open(filename, nall::file::mode::read))
                    return false;
                f.seek(off);
                f.read(into, len);
                print("Read ", len, " bytes from ", filename, "\n");
                break;
            }
            case none:
                return false;
            }
            return true;
        }
        
        uint8_t *buffer;
        nall::stream *strm;
        nall::string filename;
        unsigned off, len;
        reftype type;
    };

    struct entry {
        nall::string name;

        dataref data;
        nall::vector<entry> children;
    };

    entry root;

    inline unsigned fileCount();

    inline bool read(nall::stream *strm);
    inline bool write(nall::stream *strm, bool writeData = true);

    inline fst();
    inline ~fst();

protected:
    inline static nall::string grabFilename(nall::stream *strm, int offset);
    inline void recursiveRead(nall::stream *strm, fst::entry &node);
    inline void recursiveWrite(nall::stream *strm, fst::entry &node, unsigned *strOffset, unsigned *dataOffset, bool writeData);
    inline void recursivePreflight(fst::entry &node, unsigned *fileCount, unsigned *strTableSize = 0);

    unsigned fstOffset;
    unsigned strTableOffset;
    unsigned currEntry;
};

// I need a more clever way to do this so it's not so big...
nall::string fst::grabFilename(nall::stream *strm, int offset) {
    unsigned oldoffset = strm->offset();
    char str[256] = {0};
    char ch = '\0';

    strm->seek(offset);
    for(int i = 0; i < 256; i++) {
        ch = strm->read();
        str[i] = ch;

        if(!ch) break;
    }

    strm->seek(oldoffset);
    return nall::string(str);
}

void fst::recursiveRead(nall::stream *strm, fst::entry &node) {
    unsigned flags = strm->read();
    unsigned fnoffset = strm->readm(3);
    unsigned offset = strm->readm(4);
    unsigned length = strm->readm(4);

    ++currEntry;

    node.name = grabFilename(strm, strTableOffset + fnoffset);

    if(flags == 1) {
        while(currEntry < length) {
            fst::entry file;
            recursiveRead(strm, file);

            node.children.append(file);
        }
    } else {
        node.data = fst::dataref(strm, offset, length);
    }
}

void fst::recursiveWrite(nall::stream *strm, fst::entry &node, unsigned *strOffset, unsigned *dataOffset, bool writeData) {
    bool isDir = node.children.size() > 0;
    unsigned totalChildren = 0;
    recursivePreflight(node, &totalChildren);

    strm->writem(isDir ? 1 : 0, 1);
    strm->writem(*strOffset - strTableOffset, 3);
    *dataOffset = (*dataOffset + (4096 - 1)) & -4096;
    strm->writem(isDir ? 0 : *dataOffset, 4);
    strm->writem(isDir ? totalChildren + currEntry : node.data.len, 4);
    ++currEntry;

    int oldOffset = strm->offset();

    // write to string table
    strm->seek(*strOffset);
    for(char ch : node.name)
        strm->writem((uint8_t)ch, 1);
    strm->writem(0, 1);
    *strOffset += node.name.length() + 1;

    // write data to data offset
    if(writeData && !isDir && node.data.len > 0) {
        strm->seek(*dataOffset);
        uint8_t *buffer = new uint8_t[node.data.len];
        node.data.read(buffer);
        strm->write(buffer, node.data.len);
        delete[] buffer;
    }
    *dataOffset += node.data.len;

    // return to fst
    strm->seek(oldOffset);

    for(entry &child : node.children)
        recursiveWrite(strm, child, strOffset, dataOffset, writeData);
}

void fst::recursivePreflight(fst::entry &node, unsigned *fileCount, unsigned *strTableSize) {
    *fileCount += 1;

    if(*fileCount > 1 && strTableSize)
        strTableSize += node.name.length() + 1;

    for(entry &child : node.children)
        recursivePreflight(child, fileCount, strTableSize);
}

unsigned fst::fileCount() {
	unsigned count = 0;
	recursivePreflight(root, &count);
	return count;
}

bool fst::read(nall::stream *strm) {
    uint32_t fileCount;
    fstOffset = strm->offset();

    // read root entry
    if(strm->read() != 1)
        nall::print("warning: unexpected root flag\n");
    if(strm->readm(3) != 0)
        nall::print("warning: unexpected root string offset\n");
    if(strm->readm(4) != 0)
        nall::print("warning: unexpected root offset\n");

    fileCount = strm->readm(4);
    strTableOffset = fileCount * 0xC + fstOffset;

    currEntry = 1;

    while(currEntry < fileCount) {
        fst::entry file;
        recursiveRead(strm, file);

        root.children.append(file);
    }

    return true;
}

bool fst::write(nall::stream *strm, bool writeData) {
    unsigned fileCount = 0, strTableSize = 0, strTablePtr = 0, dataPtr = 0;
    fstOffset = strm->offset();

    if(!strm->writable())
        return false;

    recursivePreflight(root, &fileCount, &strTableSize);

    // write root entry
    strm->writem(1, 1);
    strm->writem(0, 3);
    strm->writem(0, 4);
    strm->writem(fileCount, 4);

    currEntry = 1;
    strTableOffset = fstOffset + fileCount * 0xc;
    strTablePtr = strTableOffset;
    dataPtr = strTableOffset + strTableSize;
    dataPtr = (dataPtr + (4096 - 1)) & -4096;

    for(entry e : root.children)
        recursiveWrite(strm, e, &strTablePtr, &dataPtr, writeData);

    return true;
}

fst::fst() {
}

fst::~fst() {
}
