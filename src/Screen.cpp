#include "Screen.h"

#include <cstdint>
#include <type_traits>
#include <SDL2/SDL.h>

#include "FileSystemUtils.h"
#include "Game.h"
#include "GraphicsUtil.h"
#include "Vlogging.h"

#include "GPU.h"
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
    scalingMode = ssFourToThree;
    linearFilter = true;
    badSignal = false;
}

void Screen::init(const ScreenSettings& settings)
{
    isWindowed = !settings.fullscreen;
    scalingMode = settings.scalingMode;
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

    gpu::init();
    _screenTexture.init(SCREEN_WIDTH, SCREEN_HEIGHT, "screen texture");

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

    switch (scalingMode) {
    case ssOneToOne: break; // No scaling applied
    case ssFourToThree: // Fit to height
        hscale = vscale = float(DISPLAY_HEIGHT) / float(SCREEN_HEIGHT);
        break;
    case ssFullscreen: // Fit to screen
        hscale = float(DISPLAY_WIDTH) / float(SCREEN_WIDTH);
        vscale = float(DISPLAY_HEIGHT) / float(SCREEN_HEIGHT);
        break;
    case ss_Last: break; // Cannot happen
    }

    float width = hscale * SCREEN_WIDTH;
    float height = vscale * SCREEN_HEIGHT;
    return {
        int((float(DISPLAY_WIDTH) - width) / 2.0f),
        int((float(DISPLAY_HEIGHT) - height) / 2.0f),
        int(width),
        int(height),
    };
}

void Screen::FlipScreen(const bool flipmode)
{
    // TODO(PSP): Fleeeep
    // Implement flip mode.

    gpu::start();

    gpu::swap();
    gpu::drawToScreen();

    gpu::clear({0, 0, 0});

    _screenTexture.upload(SCREEN_WIDTH_VRAM, _screenData);

    const auto filter = isFiltered ? gpu::tfLinear : gpu::tfNearest;
    const auto sampler = _screenTexture.sampler()
        .withFilter(filter);
    gpu::blit(sampler, screenRect());

    gpu::end();
    gpu::waitVblank();
}

void Screen::toggleScalingMode(void)
{
    scalingMode = ScreenScaling((int(scalingMode) + 1) % int(ss_Last));
}

void Screen::toggleLinearFilter(void)
{
    isFiltered = !isFiltered;
}
