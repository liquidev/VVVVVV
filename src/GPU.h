// Hardware rendering abstraction.

#ifndef GPU_H
#define GPU_H

#include <SDL2/SDL.h>
#include <cstdint>

#include "VRAM.h"

namespace gpu {

enum TextureFilter {
    tfLinear,
    tfNearest,
};

class Texture;

class Sampler
{
    friend class Texture;
    friend void blit(const Sampler &, const SDL_Rect &, const SDL_Rect &);

    const Texture &_tex;
    TextureFilter _filter;

    Sampler(const Texture &tex);

    void bind() const;

public:
    const Texture &texture() const;

    Sampler withFilter(TextureFilter filter) const;
};

// An RGBA texture.
class Texture
{
    friend class Sampler;

    vram::Allocation _vram;
    unsigned _vramWidth, _vramHeight;
    unsigned _width, _height;

public:
    Texture();
    Texture(unsigned w, unsigned h, const char *what = "gpu::Texture");

    unsigned width() const;
    unsigned height() const;

    // Uploads texture data to the GPU. Must be called while in a batch.
    void upload(unsigned dataWidth, void *data);

    Sampler sampler() const;
    operator Sampler() const;
};

struct Color
{
    uint8_t r, g, b, a;

    inline static Color unpack(uint32_t packed)
    {
        return {
            uint8_t(packed & 0xFF),         // r
            uint8_t((packed >> 8) & 0xFF),  // g
            uint8_t((packed >> 16) & 0xFF), // b
            uint8_t((packed >> 24) & 0xFF), // a
        };
    }

    inline uint32_t pack()
    {
        return
            uint32_t(r) |
            (uint32_t(g) << 8) |
            (uint32_t(b) << 16) |
            (uint32_t(a) << 24)
            ;
    }
};

// Initializes GPU state.
void init();

// Swaps front and back buffers. Must be called at the beginning of a frame.
void swap();

// Begins a new batch.
void start();

// Clears the screen with a color. Must be used in a batch.
void clear(Color color);

// Blits a texture to the screen using the given sampler.
void blit(const Sampler &smp, const SDL_Rect &position, const SDL_Rect &uv);

// Same as the other `blit` but blits the entire texture and not just a fragment.
void blit(const Sampler &smp, const SDL_Rect &position);

// Ends the current batch.
void end();

// Waits for vertical synchronization, in order to reduce tearing.
// Must be called at the end of a frame.
void waitVblank();

}

#endif
