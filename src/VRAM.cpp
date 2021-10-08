#include "VRAM.h"

#include <pspge.h>

#include "Screen.h"
#include "Vlogging.h"

// An allocator for chunks of VRAM.
// All VRAM allocations are static. Everything is allocated once, and never freed.
struct Allocator {
    size_t origin;
    size_t position;

    Allocator(void *_initial_position)
    : origin((size_t)_initial_position)
    , position((size_t)_initial_position)
    {
    }

    inline vram::Allocation next(const char *what, size_t size) {
        size_t loc = position;
        position += size;
        if (position - origin > sceGeEdramGetSize()) {
            vlog_error("Out of VRAM (allocation '%s')", what);
        }
        // Align position to 16 bytes.
        position = (position + 15) & ~15;
        return vram::Allocation{
            (void *)loc,
            size,
        };
    }
};

namespace vram {
    Allocation display;
    Allocation depth;
    Allocation screenBuffer;

    void init() {
        Allocator allocator(sceGeEdramGetAddr());

        const size_t display_size = DISPLAY_WIDTH_POT * DISPLAY_HEIGHT;
        display = allocator.next("display front buffer", display_size * 4);
        depth = allocator.next("display depth buffer", display_size * 2);
        screenBuffer = allocator.next("game screen texture", SCREEN_WIDTH_VRAM * SCREEN_HEIGHT_VRAM * SCREEN_CHANNELS);

        vlog_info("display front address: %p", display.ptr);
        vlog_info(" screenBuffer address: %p", screenBuffer.ptr);
    }
}
