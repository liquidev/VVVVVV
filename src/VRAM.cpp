#include "VRAM.h"

#include <pspge.h>

#include "Vlogging.h"

// An allocator for chunks of VRAM.
// All VRAM allocations are static. Everything is allocated once, and never freed.
struct Allocator {
    size_t origin;
    size_t position;
    size_t count;

    Allocator(void *_initial_position)
    : origin((size_t)_initial_position)
    , position((size_t)_initial_position)
    , count(0)
    {
    }

    inline vram::Allocation next(const char *what, size_t size)
    {
        size_t loc = position;
        size_t vram_size = sceGeEdramGetSize();
        position += size;
        count += size;
        if (size > vram_size) {
            vlog_error("Out of VRAM (allocation '%s')", what);
        } else {
            vlog_debug("VRAM allocation '%s' at %p", what, (void *)loc);
            vlog_debug("VRAM used: %u / %u (%.1f%%)",
                (unsigned)count, (unsigned)vram_size,
                float(count) / float(vram_size) * 100.0f);
        }
        // Align position to 16 bytes.
        position = (position + 15) & ~15;
        return vram::Allocation{
            (void *)loc,
            size,
        };
    }
};

static Allocator allocator{nullptr};

namespace vram {
    Allocation display;
    Allocation depth;
    Allocation screenBuffer;

    void init() {
        allocator = Allocator(sceGeEdramGetAddr());
    }

    Allocation allocate(const char *what, size_t size)
    {
        return allocator.next(what, size);
    }

    Allocation allocateTexture16(const char *what, size_t width, size_t height)
    {
        return allocate(what, width * height * 2);
    }

    Allocation allocateTexture32(const char *what, size_t width, size_t height)
    {
        return allocate(what, width * height * 4);
    }
}
