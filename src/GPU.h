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

class Framebuffer;

// An RGBA texture.
// Textures are only as a means of allocating Framebuffers, which store an area on the texture.
class Texture
{
    friend class Sampler;
    friend void init();
    friend void swap();
    friend void drawTo(gpu::Framebuffer &fb);

    vram::Allocation _vram;
    unsigned _vramWidth, _vramHeight;
    unsigned _width, _height;

public:
    Texture();
    void init(unsigned w, unsigned h, const char *what = "gpu::Texture");

    // Texture(const Texture &) = delete;
    // Texture &operator=(const Texture &) = delete;

    unsigned width() const;
    unsigned height() const;

    // Uploads texture data to the GPU. Must be called while in a batch.
    void upload(unsigned dataWidth, void *data);

    Sampler sampler() const;
    operator Sampler() const;
};

// A framebuffer, which is a slice of an existing RGBA texture.
class Framebuffer
{
    friend void drawTo(gpu::Framebuffer &fb);

    const Texture *_tex;
    uint16_t _x, _y, _width, _height;

public:
    Framebuffer();
    void init(const Texture &tex, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
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

// Makes it so that drawing should be done on the main screen.
void drawToScreen();

// Makes it so that all drawing is done to the given framebuffer.
void drawTo(gpu::Framebuffer &fb);

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
