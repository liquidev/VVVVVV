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

ScreenSettings::ScreenSettings(void)
{
    windowWidth = 320;
    windowHeight = 240;
    fullscreen = false;
    useVsync = true; // Now that uncapped is the default...
    stretch = 0;
    linearFilter = false;
    badSignal = false;
}

void Screen::init(const ScreenSettings& settings)
{
    m_screen = NULL;
    isWindowed = !settings.fullscreen;
    stretchMode = settings.stretch;
    isFiltered = settings.linearFilter;
    vsync = settings.useVsync;
    filterSubrect.x = 1;
    filterSubrect.y = 1;
    filterSubrect.w = 318;
    filterSubrect.h = 238;

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
    sceGuDrawBuffer(GU_PSM_8888, vram::display, DISPLAY_WIDTH_POT);
    sceGuDispBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT, vram::display, DISPLAY_WIDTH_POT);
    sceGuDepthBuffer(vram::depth, 512);
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

    sceDisplaySetMode(0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    sceDisplaySetFrameBuf(
        vram::display,
        DISPLAY_WIDTH_POT,
        PSP_DISPLAY_PIXEL_FORMAT_8888,
        PSP_DISPLAY_SETBUF_NEXTFRAME
    );

    badSignalEffect = settings.badSignal;
}

void Screen::destroy(void)
{
}

void Screen::GetSettings(ScreenSettings* settings)
{
    int width, height;
    GetWindowSize(&width, &height);

    settings->windowWidth = width;
    settings->windowHeight = height;

    settings->fullscreen = !isWindowed;
    settings->useVsync = vsync;
    settings->stretch = stretchMode;
    settings->linearFilter = isFiltered;
    settings->badSignal = badSignalEffect;
}

void Screen::ResizeScreen(int x, int y)
{
    // TODO
}

void Screen::ResizeToNearestMultiple(void)
{
    // TODO
}

void Screen::GetWindowSize(int* x, int* y)
{
    *x = DISPLAY_WIDTH;
    *y = DISPLAY_HEIGHT;
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

static int _frame_count = 0;

void Screen::FlipScreen(const bool flipmode)
{
    SDL_RendererFlip flip_flags;
    if (flipmode)
    {
        flip_flags = SDL_FLIP_VERTICAL;
    }
    else
    {
        flip_flags = SDL_FLIP_NONE;
    }

    // TODO(PSP): Fleeeep

    sceGuStart(GU_DIRECT, _drawList);

    sceGuClearColor(0xFF0000);
    sceGuClear(GU_COLOR_BUFFER_BIT);

    struct Vertex {
        uint16_t u, v;
        int16_t x, y, z;
        // uint8_t _pad[2];
    };
    Vertex *verts = (Vertex *)sceGuGetMemory(2 * sizeof(Vertex));
    verts[0] = { 0, 0,           0, 0, 0 };
    verts[1] = { 65535, 65535,   512, 256, 0 };

    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            int i = (x + y * SCREEN_WIDTH_VRAM) * 4;
            _screenData[i] = x;
            _screenData[i + 1] = y;
            _screenData[i + 2] = 0;
            _screenData[i + 3] = 0;
        }
    }

    sceGuCopyImage(
        GU_PSM_8888,
        0, 0, SCREEN_WIDTH_VRAM, SCREEN_HEIGHT_VRAM,
        SCREEN_WIDTH_VRAM, _screenData,
        0, 0,
        SCREEN_WIDTH_VRAM, vram::screenBuffer
    );
    sceGuTexMode(GU_PSM_8888, 1, 0, false);
    sceGuTexImage(0, SCREEN_WIDTH_VRAM, SCREEN_HEIGHT_VRAM, SCREEN_WIDTH_VRAM, vram::screenBuffer);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuDrawArray(
        GU_SPRITES,
        GU_TRANSFORM_2D | GU_TEXTURE_16BIT | GU_VERTEX_16BIT,
        2,
        nullptr,
        (void *)verts
    );

    ++_frame_count;
    if (_frame_count == 60 * 3) {
        sceGuClearColor(0xFFFFFF);
        sceGuClear(GU_COLOR_BUFFER_BIT);
        SDL_SaveBMP(m_screen, "/conk_conk.bmp");
    }

    sceGuTexSync();
    sceGuFinish();
    sceGuSync(0, 0);

    pspDebugScreenSetOffset((int)(size_t)vram::screenBuffer.ptr);
    pspDebugScreenSetXY(0, 0);

    sceDisplayWaitVblank();
}

void Screen::toggleFullScreen(void)
{
    isWindowed = !isWindowed;
    ResizeScreen(-1, -1);

    if (game.currentmenuname == Menu::graphicoptions)
    {
        /* Recreate menu to update "resize to nearest" */
        game.createmenu(game.currentmenuname, true);
    }
}

void Screen::toggleStretchMode(void)
{
    stretchMode = (stretchMode + 1) % 3;
    ResizeScreen(-1, -1);
}

void Screen::toggleLinearFilter(void)
{
    isFiltered = !isFiltered;
}

void Screen::toggleVSync(void)
{
#if SDL_VERSION_ATLEAST(2, 0, 17)
    vsync = !vsync;
    SDL_RenderSetVSync(m_renderer, (int) vsync);
#endif
}
