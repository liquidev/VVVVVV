// .psp (VPSP) file support.

// VPSP is a dead-simple binary format optimized for quick lookups of data, while still using a
// single FILE * handle.

#ifndef VPSPFILE_H
#define VPSPFILE_H

#include <cstdint>
#include <cstdio>

constexpr int VPSP_MAX_FILES = 4;
constexpr int VPSP_MAX_SIZE = 512 * 1024;

// A file in a VPSP archive.
struct MemFile {
    int _open_index; // private

    uint8_t *data;
    unsigned length;

    MemFile()
    : data(nullptr)
    , length(0)
    {
    }
};

namespace VPSP {

    FILE *getMount();

    struct Entry {
        constexpr static int PathLength = 40;

        // The current longest filename in data.zip is 33 chars long.
        char path[PathLength];
        unsigned path_length;
        unsigned file_length;
        unsigned file_offset;
    };

    // Mounts a .psp file as the current file.
    void mount(const char *path);

    // Opens a file for reading.
    //
    // A maximum of VPSP_MAX_FILES can be opened at once; if that limit is
    // exceeded, the game will quit. Additionally, each file may only be
    // VPSP_MAX_SIZE bytes long.
    MemFile openMemFile(const char *path);
};

#endif
