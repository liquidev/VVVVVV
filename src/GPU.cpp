#include "GPU.h"

#include <SDL2/SDL_rect.h>
#include <cstdint>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <utility>

#include "Screen.h"
#include "VRAM.h"
#include "Vlogging.h"

static vram::Allocation displayFront;
static vram::Allocation displayBack;
static vram::Allocation depth;

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

gpu::Sampler::Sampler(const Texture &tex)
: _tex(tex)
, _filter(gpu::tfLinear)
{
}

const gpu::Texture &gpu::Sampler::texture() const
{
    return _tex;
}

gpu::Sampler gpu::Sampler::withFilter(gpu::TextureFilter filter) const
{
    gpu::Sampler copy = *this;
    copy._filter = filter;
    return copy;
}

void gpu::Sampler::bind() const
{
    sceGuTexImage(0, _tex._vramWidth, _tex._vramHeight, _tex._vramWidth, _tex._vram.ptr);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    int filter;
    switch (_filter) {
    case gpu::tfNearest: filter = GU_NEAREST; break;
    case gpu::tfLinear:  filter = GU_LINEAR; break;
    }
    sceGuTexFilter(filter, filter);
}

// Texture

gpu::Texture::Texture() {}

gpu::Texture::Texture(unsigned w, unsigned h, const char *what)
{
    _vramWidth = nextPowerOfTwo(w);
    _vramHeight = nextPowerOfTwo(h);
    _width = w;
    _height = h;
    _vram = vram::allocateTexture32(what, _vramWidth, _vramHeight);
}

unsigned gpu::Texture::width() const
{
    return _width;
}

unsigned gpu::Texture::height() const
{
    return _height;
}

void gpu::Texture::upload(unsigned dataWidth, void *data)
{
    sceGuCopyImage(GU_PSM_8888, 0, 0, _width, _height, dataWidth, data, 0, 0, _vramWidth, _vram);
    sceGuTexSync();
}

gpu::Sampler gpu::Texture::sampler() const
{
    return gpu::Sampler(*this);
}

gpu::Texture::operator Sampler() const
{
    return sampler();
}

// gpu

void gpu::init()
{
    displayFront = vram::allocateTexture32("display front buffer", DISPLAY_WIDTH_POT, DISPLAY_HEIGHT);
    displayBack = vram::allocateTexture32("display back buffer", DISPLAY_WIDTH_POT, DISPLAY_HEIGHT);
    depth = vram::allocateTexture16("depth buffer", DISPLAY_WIDTH_POT, DISPLAY_HEIGHT);

    sceGuInit();

    sceGuStart(GU_DIRECT, drawList);

    // Buffers
    sceGuDrawBuffer(GU_PSM_8888, displayBack, DISPLAY_WIDTH_POT);
    sceGuDispBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT, displayFront, DISPLAY_WIDTH_POT);
    sceGuDepthBuffer(depth, DISPLAY_WIDTH_POT);
    // Offsets
    sceGuOffset(2048 - (DISPLAY_WIDTH/2), 2048 - (DISPLAY_HEIGHT/2));
    sceGuViewport(2048, 2048, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    // Flags
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuFrontFace(GU_CW);
    sceGuEnable(GU_TEXTURE_2D);
    // Textures
    sceGuTexMode(GU_PSM_8888, 1, 0, false);

    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(true);
}

void gpu::swap()
{
    std::swap(displayFront, displayBack);
    sceGuDrawBuffer(GU_PSM_8888, displayBack, DISPLAY_WIDTH_POT);
    sceGuDispBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT, displayFront, DISPLAY_WIDTH_POT);
    sceDisplaySetFrameBuf(displayFront, DISPLAY_WIDTH_POT, GU_PSM_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);
}

void gpu::start()
{
    assert(!inBatch, "attempt to start() while already in a batch");
    inBatch = true;
    sceGuStart(GU_DIRECT, drawList);
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

struct Vertex
{
    constexpr static unsigned FORMAT = GU_TEXTURE_16BIT | GU_VERTEX_16BIT;

    uint16_t u, v;
    int16_t x, y, z;
};

void gpu::blit(const Sampler &smp, const SDL_Rect &position, const SDL_Rect &uv)
{
    smp.bind();

    Vertex *verts = (Vertex *)sceGuGetMemory(2 * sizeof(Vertex));
    int16_t
        x1 = position.x,
        y1 = position.y,
        x2 = position.x + position.w,
        y2 = position.y + position.h;
    uint16_t
        uvx1 = uv.x,
        uvy1 = uv.y,
        uvx2 = uv.x + uv.w,
        uvy2 = uv.y + uv.h;
    verts[0] = { uvx1, uvy1,  x1, y1, 0 };
    verts[1] = { uvx2, uvy2,  x2, y2, 0 };

    sceGuDrawArray(
        GU_SPRITES,
        GU_TRANSFORM_2D | GU_TEXTURE_16BIT | GU_VERTEX_16BIT,
        2,
        nullptr,
        (void *)verts
    );
}

void gpu::blit(const Sampler &smp, const SDL_Rect &position)
{
    gpu::blit(smp, position, {0, 0, int(smp.texture().width()), int(smp.texture().height())});
}

void gpu::end()
{
    assert(inBatch, "attempt to end() while not in a batch");
    inBatch = false;
    sceGuFinish();
    sceGuSync(0, 0);
}

void gpu::waitVblank()
{
    sceDisplayWaitVblankStart();
}
