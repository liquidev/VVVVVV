#include "Screen.h"

#include <cstdint>
#include <type_traits>
#include <SDL2/SDL.h>

#include "FileSystemUtils.h"
#include "Game.h"
#include "GraphicsUtil.h"
#include "Vlogging.h"

#include "VRAM.h"
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspdebug.h>
#include <psputils.h>
#include <utility>

ScreenSettings::ScreenSettings(void)
{
    windowWidth = 320;
    windowHeight = 240;
    fullscreen = false;
    useVsync = true; // Now that uncapped is the default...
    scalingMode = ssOneToOne;
    linearFilter = false;
    badSignal = false;
}

void Screen::init(const ScreenSettings& settings)
{
    m_screen = NULL;
    isWindowed = !settings.fullscreen;
    scalingMode = settings.scalingMode;
    isFiltered = settings.linearFilter;
    vsync = settings.useVsync;
    filterSubrect.x = 1;
    filterSubrect.y = 1;
    filterSubrect.w = 318;
    filterSubrect.h = 238;

    displayFront = vram::allocateTexture32("display front buffer", DISPLAY_WIDTH_POT, DISPLAY_HEIGHT);
    displayBack = vram::allocateTexture32("display back buffer", DISPLAY_WIDTH_POT, DISPLAY_HEIGHT);
    depth = vram::allocateTexture16("depth buffer", DISPLAY_WIDTH_POT, DISPLAY_HEIGHT);
    screenTexture = vram::allocateTexture32("screen texture", SCREEN_WIDTH_VRAM, SCREEN_HEIGHT_VRAM);
    drawBuffer = displayBack;

    m_screen = SDL_CreateRGBSurfaceFrom(
        (void *)&_screenData,
        SCREEN_WIDTH_VRAM,
        SCREEN_HEIGHT_VRAM,
        32,
        sizeof(Uint8) * SCREEN_WIDTH_VRAM * 4,
        0x000000FF,
        0x0000FF00,
        0x00FF0000,
        0xFF000000
    );

    sceGuInit();

    sceGuStart(GU_DIRECT, _drawList);

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

    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(true);

    // sceDisplaySetMode(0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    badSignalEffect = settings.badSignal;
}

void Screen::destroy(void)
{
}

void Screen::GetSettings(ScreenSettings* settings)
{
    settings->windowWidth = DISPLAY_WIDTH;
    settings->windowHeight = DISPLAY_HEIGHT;

    settings->fullscreen = !isWindowed;
    settings->useVsync = vsync;
    settings->scalingMode = scalingMode;
    settings->linearFilter = isFiltered;
    settings->badSignal = badSignalEffect;
}

void Screen::UpdateScreen(SDL_Surface* buffer, SDL_Rect* rect)
{
    if((buffer == NULL) && (m_screen == NULL))
    {
        return;
    }

    if(badSignalEffect)
    {
        buffer = ApplyFilter(buffer);
    }


    ClearSurface(m_screen);
    BlitSurfaceStandard(buffer,NULL,m_screen,rect);

    if(badSignalEffect)
    {
        SDL_FreeSurface(buffer);
    }

}

const SDL_PixelFormat* Screen::GetFormat(void)
{
    return m_screen->format;
}

SDL_Rect Screen::screenRect() {
    float hscale = 1.0f, vscale = 1.0f;

    float width = hscale * SCREEN_WIDTH;
    float height = vscale * SCREEN_HEIGHT;
    return {
        int((float(DISPLAY_WIDTH) - width) / 2.0f),
        int((float(DISPLAY_HEIGHT) - height) / 2.0f),
        int(width),
        int(height),
    };
}

static void drawRectangle(const SDL_Rect &position, const SDL_Rect &uv)
{
    struct Vertex {
        uint16_t u, v;
        int16_t x, y, z;
    };
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

void Screen::FlipScreen(const bool flipmode)
{
    // TODO(PSP): Fleeeep
    // Implement flip mode.

    sceGuStart(GU_DIRECT, _drawList);

    // Do our own buffer swapping procedure because sceGuSwapBuffers doesn't work.
    std::swap(displayFront, displayBack);
    sceGuDrawBuffer(GU_PSM_8888, displayBack, DISPLAY_WIDTH_POT);
    sceGuDispBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT, displayFront, DISPLAY_WIDTH_POT);
    sceDisplaySetFrameBuf(displayFront, DISPLAY_WIDTH_POT, GU_PSM_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);

    // sceGuClearColor(0x000000);
    sceGuClear(GU_COLOR_BUFFER_BIT);

    sceGuCopyImage(
        GU_PSM_8888,
        0, 0, SCREEN_WIDTH_VRAM, SCREEN_HEIGHT_VRAM,
        SCREEN_WIDTH_VRAM, _screenData,
        0, 0,
        SCREEN_WIDTH_VRAM, screenTexture
    );
    sceGuTexSync();

    sceGuTexMode(GU_PSM_8888, 1, 0, false);
    sceGuTexImage(0, SCREEN_WIDTH_VRAM, SCREEN_HEIGHT_VRAM, SCREEN_WIDTH_VRAM, screenTexture);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    int filter = isFiltered ? GU_LINEAR : GU_NEAREST;
    sceGuTexFilter(filter, filter);
    drawRectangle(screenRect(), {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT});

    sceGuFinish();
    sceGuSync(0, 0);

    // Swap buffers
    sceDisplayWaitVblankStart();
}

void Screen::toggleScalingMode(void)
{
    scalingMode = ScreenScaling((int(scalingMode) + 1) % int(ss_Last));
}

void Screen::toggleLinearFilter(void)
{
    isFiltered = !isFiltered;
}
