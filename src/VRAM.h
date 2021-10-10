// VRAM calculations.

#ifndef VRAM_H
#define VRAM_H

#include <cstddef>

namespace vram {
    struct Allocation {
        void *ptr;
        size_t size;

        // Returns the relative address of this allocation (starting from 0x00000000).
        // This is required when passing framebuffer pointers into GU functions.
        void *absolute() const;

        operator void *() const {
            return ptr;
        }
    };

    void init();

    Allocation allocate(const char *what, size_t size);
    Allocation allocateTexture16(const char *what, size_t width, size_t height);
    Allocation allocateTexture32(const char *what, size_t width, size_t height);
}

#endif
