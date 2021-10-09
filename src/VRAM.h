// VRAM calculations.

#ifndef VRAM_H
#define VRAM_H

#include <cstddef>

namespace vram {
    struct Allocation {
        void *ptr;
        size_t size;

        operator void *() {
            return ptr;
        }
    };

    void init();

    Allocation allocate(const char *what, size_t size);
    Allocation allocateTexture16(const char *what, size_t width, size_t height);
    Allocation allocateTexture32(const char *what, size_t width, size_t height);
}

#endif
