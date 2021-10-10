#include "GPU.h"

#include <SDL2/SDL_rect.h>
#include <cstdint>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <utility>

#include "Exit.h"
#include "Screen.h"
#include "VRAM.h"
#include "Vlogging.h"
#include "pspge.h"
#include "psputils.h"

static gpu::Texture displayFrontTex;
static gpu::Texture displayBackTex;

static gpu::Framebuffer frontBuffer;
static gpu::Framebuffer backBuffer;
static gpu::Framebuffer *drawBuffer = nullptr;

// General-purpose framebuffer storage
static gpu::Texture gpFramebuffers;

static uint8_t drawList[1024]; // Increase size if needed

static bool inBatch = false;

// common

static inline void assert(bool cond, const char *message)
{
    if (!cond) {
        pspDebugScreenClear();
        vlog_error("(GPU) %s", message);
    }
}

static inline unsigned nextPowerOfTwo(unsigned v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

// Sampler

gpu::Sampler::Sampler(const Framebuffer &fb)
: _fb(fb)
, _filter(gpu::tfLinear)
{
}

const gpu::Framebuffer &gpu::Sampler::framebuffer() const
{
    return _fb;
}

gpu::Sampler gpu::Sampler::withFilter(gpu::TextureFilter filter) const
{
    gpu::Sampler copy = *this;
    copy._filter = filter;
    return copy;
}

void gpu::Sampler::bind() const
{
    const auto &tex = _fb.texture();
    sceGuTexImage(0, tex._vramWidth, tex._vramHeight, tex._vramWidth, tex._vram.absolute());
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    int filter;
    switch (_filter) {
    case gpu::tfNearest: filter = GU_NEAREST; break;
    case gpu::tfLinear:  filter = GU_LINEAR; break;
    }
    sceGuTexFilter(filter, filter);
}

const gpu::Texture &gpu::Sampler::texture() const
{
    return _fb.texture();
}

// Texture

gpu::Texture::Texture()
: _packer(0, 0)
{
}

void gpu::Texture::init(unsigned w, unsigned h, const char *what)
{
    _vramWidth = nextPowerOfTwo(w);
    _vramHeight = nextPowerOfTwo(h);
    _width = w;
    _height = h;
    _vram = vram::allocateTexture32(what, _vramWidth, _height);
    // The packer only distributes the part we allocate, not the part we tell the GE is ours.
    _packer = RectPacker(_vramWidth, _height);
}

unsigned gpu::Texture::width() const
{
    return _width;
}

unsigned gpu::Texture::height() const
{
    return _height;
}

bool gpu::Texture::allocate(Framebuffer &out_fb, unsigned w, unsigned h)
{
    SDL_Rect rect{0, 0, int(w), int(h)};
    if (_packer.pack(rect)) {
        out_fb.init(*this, rect.x, rect.y, rect.w, rect.h);
        return true;
    }
    return false;
}

// Framebuffer

gpu::Framebuffer::Framebuffer()
{
}

void gpu::Framebuffer::init(const Texture &tex, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    _tex = &tex;
    _x = x;
    _y = y;
    _width = w;
    _height = h;
}

const gpu::Texture &gpu::Framebuffer::texture() const
{
    return *_tex;
}

uint16_t gpu::Framebuffer::width() const
{
    return _width;
}

uint16_t gpu::Framebuffer::height() const
{
    return _height;
}

void gpu::Framebuffer::upload(unsigned dataWidth, void *data)
{
    const auto tex = texture();
    sceKernelDcacheWritebackAll();
    sceGuCopyImage(GU_PSM_8888, 0, 0, _width, _height, dataWidth, data, 0, 0, tex._vramWidth, tex._vram.absolute());
    sceGuTexSync();
}

gpu::Sampler gpu::Framebuffer::sampler() const
{
    return gpu::Sampler(*this);
}

gpu::Framebuffer::operator Sampler() const
{
    return sampler();
}

// gpu

void gpu::init()
{
    displayBackTex.init(DISPLAY_WIDTH, DISPLAY_HEIGHT, "display back buffer");
    displayFrontTex.init(DISPLAY_WIDTH, DISPLAY_HEIGHT, "display front buffer");
    gpFramebuffers.init(512, 240, "general-purpose framebuffers");

    displayFrontTex.allocate(frontBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    displayBackTex.allocate(backBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    drawBuffer = &backBuffer;

    sceGuInit();

    gpu::start();

    // Buffers
    gpu::drawTo(*drawBuffer);
    sceGuDispBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT, displayFrontTex._vram, DISPLAY_WIDTH_POT);
    sceGuDepthBuffer(0, DISPLAY_WIDTH_POT);
    sceGuDisable(GU_DEPTH_TEST);
    // Flags
    sceGuFrontFace(GU_CW);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_TEXTURE_2D);
    // Textures
    sceGuTexMode(GU_PSM_8888, 1, 0, false);

    sceGuClear(GU_COLOR_BUFFER_BIT);

    gpu::end();

    sceDisplayWaitVblankStart();
    sceGuDisplay(true);
    sceDisplaySetFrameBuf(displayFrontTex._vram.absolute(), displayBackTex._vramWidth, GU_PSM_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);
}

gpu::Framebuffer gpu::createFramebuffer(unsigned int w, unsigned int h)
{
    gpu::Framebuffer fb;

    if (gpFramebuffers.allocate(fb, w, h)) return fb;

    vlog_error("(GPU) could not allocate framebuffer: no space left on dedicated textures");
    VVV_exit(1);
}

void gpu::start()
{
    assert(!inBatch, "attempt to start() while already in a batch");
    inBatch = true;
    sceGuStart(GU_DIRECT, drawList);
}

void gpu::swap()
{
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
    std::swap(frontBuffer, backBuffer);
    drawBuffer = &backBuffer;
}

static void drawingMustHappenInBatch()
{
    assert(inBatch, "all drawing must happen in a batch");
}

void gpu::clear(Color color)
{
    drawingMustHappenInBatch();

    uint32_t col = color.pack();
    sceGuClearColor(col);
    sceGuClear(GU_COLOR_BUFFER_BIT);
}

void gpu::drawTo(gpu::Framebuffer &fb)
{
    drawingMustHappenInBatch();

    const auto tex = fb._tex;
    const auto origin_x = 2048 + fb._x;
    const auto origin_y = 2048 + fb._y;
    sceGuDrawBuffer(GU_PSM_8888, tex->_vram, tex->_vramWidth);
    // sceGuDrawBuffer(GU_PSM_8888, tex->_vram.ptr, tex->_vramWidth);
    sceGuOffset(origin_x - (tex->_width/2), origin_x - (tex->_height/2));
    sceGuViewport(origin_x, origin_y, fb._width, fb._height);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, fb._width, fb._height);
}

void gpu::drawToScreen()
{
    gpu::drawTo(*drawBuffer);
}

struct ColorVertex
{
    constexpr static unsigned Format = GU_COLOR_8888 | GU_VERTEX_16BIT;

    uint32_t color;
    int16_t x, y, z;
};

void gpu::fillRectangle(const SDL_Rect &rect, Color color)
{
    drawingMustHappenInBatch();

    const auto packed_color = color.pack();

    auto verts = (ColorVertex *)sceGuGetMemory(2 * sizeof(ColorVertex));
    int16_t
        x1 = rect.x,
        y1 = rect.y,
        x2 = rect.x + rect.w,
        y2 = rect.y + rect.h;
    verts[0] = { packed_color, x1, y1, 0 };
    verts[1] = { packed_color, x2, y2, 0 };

    sceGuDrawArray(GU_SPRITES, GU_TRANSFORM_2D | ColorVertex::Format, 2, nullptr, verts);
}

struct TextureVertex
{
    constexpr static unsigned Format = GU_TEXTURE_16BIT | GU_VERTEX_16BIT;

    uint16_t u, v;
    int16_t x, y, z;
};

void gpu::blit(const Sampler &smp, const SDL_Rect &position, const SDL_Rect &uv)
{
    drawingMustHappenInBatch();
    smp.bind();

    const auto fb = smp.framebuffer();
    auto verts = (TextureVertex *)sceGuGetMemory(2 * sizeof(TextureVertex));
    int16_t
        x1 = position.x,
        y1 = position.y,
        x2 = position.x + position.w,
        y2 = position.y + position.h;
    uint16_t
        uvx1 = fb._x + uv.x,
        uvy1 = fb._y + uv.y,
        uvx2 = fb._x + uv.x + uv.w,
        uvy2 = fb._y + uv.y + uv.h;
    verts[0] = { uvx1, uvy1,  x1, y1, 0 };
    verts[1] = { uvx2, uvy2,  x2, y2, 0 };

    sceGuDrawArray(GU_SPRITES, GU_TRANSFORM_2D | TextureVertex::Format, 2, nullptr, verts);
}

void gpu::blit(const Sampler &smp, const SDL_Rect &position)
{
    gpu::blit(smp, position, {0, 0, int(smp.framebuffer().width()), int(smp.framebuffer().height())});
}

void gpu::end()
{
    assert(inBatch, "attempt to end() while not in a batch");
    inBatch = false;
    sceGuFinish();
    sceGuSync(0, 0);
}
