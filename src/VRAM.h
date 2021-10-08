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

    extern Allocation display;
    extern Allocation depth;
    extern Allocation screenBuffer;
}

#endif
