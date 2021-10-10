// Hardware rendering abstraction.

// The public interface can assume that none of the methods fail, in order to simplify code.
// When they do fail however, they take down the whole game with them, so beware and test against
// all edge cases.

#ifndef GPU_H
#define GPU_H

#include <SDL2/SDL.h>
#include <cstdint>

#include "RectPacker.h"
#include "VRAM.h"

namespace gpu {

enum TextureFilter {
    tfLinear,
    tfNearest,
};

class Texture;
class Framebuffer;

class Sampler
{
    friend class Framebuffer;
    friend void blit(const Sampler &, const SDL_Rect &, const SDL_Rect &);

    const Framebuffer &_fb;
    TextureFilter _filter;

    Sampler(const Framebuffer &tex);

    void bind() const;

public:
    const Texture &texture() const;
    const Framebuffer &framebuffer() const;

    Sampler withFilter(TextureFilter filter) const;
};

class Framebuffer;

// An RGBA texture.
// Textures are only as a means of allocating Framebuffers, which store an area on the texture.
class Texture
{
    friend class Framebuffer;
    friend class Sampler;
    friend void init();
    friend void swap();
    friend void drawTo(gpu::Framebuffer &fb);

    RectPacker _packer;
    vram::Allocation _vram;
    unsigned _vramWidth, _vramHeight;
    unsigned _width, _height;

public:
    Texture();

    // Initializes the texture.
    //
    // The width that's actually allocated is brought upward to the next power of to, so eg. a 320px
    // wide texture allocates 512 * h * 4 bytes.
    void init(unsigned w, unsigned h, const char *what = "gpu::Texture");

    unsigned width() const;
    unsigned height() const;

    // Allocates a new framebuffer on the texture.
    bool allocate(Framebuffer &out_fb, unsigned w, unsigned h);
};

// A framebuffer, which is a slice of an existing RGBA texture.
class Framebuffer
{
    friend void drawTo(gpu::Framebuffer &fb);
    friend void blit(const Sampler &smp, const SDL_Rect &position, const SDL_Rect &uv);

    const Texture *_tex;
    uint16_t _x, _y, _width, _height;

public:
    Framebuffer();
    void init(const Texture &tex, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

    const Texture &texture() const;
    uint16_t width() const;
    uint16_t height() const;

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

// Creates a new framebuffer with the specified size.
Framebuffer createFramebuffer(unsigned w, unsigned h);

// Begins a new batch.
void start();

// Swaps front and back buffers. Must be called at the end of a frame.
void swap();

// Clears the screen with a color. Must be used in a batch.
void clear(Color color);

// Makes it so that drawing should be done on the main screen.
void drawToScreen();

// Makes it so that all drawing is done to the given framebuffer.
void drawTo(gpu::Framebuffer &fb);

// Draws a filled rectangle.
void fillRectangle(const SDL_Rect &rect, Color color);

// Blits a texture to the screen using the given sampler.
void blit(const Sampler &smp, const SDL_Rect &position, const SDL_Rect &uv);

// Same as the other `blit` but blits the entire texture and not just a fragment.
void blit(const Sampler &smp, const SDL_Rect &position);

// Ends the current batch.
void end();

}

#endif
