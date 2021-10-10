#ifndef SCREEN_H
#define SCREEN_H

#include <SDL2/SDL.h>
#include <cstdint>

#include "GPU.h"
#include "ScreenSettings.h"
#include "VRAM.h"

static const int SCREEN_WIDTH = 320;
static const int SCREEN_HEIGHT = 240;

// PSP only supports power-of-two texture sizes.
static const int SCREEN_WIDTH_VRAM = 512;
static const int SCREEN_HEIGHT_VRAM = 256;
static const int SCREEN_CHANNELS = 4;

// PSP display size
static const int DISPLAY_WIDTH = 480;
static const int DISPLAY_HEIGHT = 272;
// Display width, but it's a power of 2.
static const int DISPLAY_WIDTH_POT = 512;

class Screen
{
public:
    void init(const ScreenSettings& settings);
    void destroy(void);

    void GetSettings(ScreenSettings* settings);

    // PSP doesn't have window icons.
    // void LoadIcon(void);

    SDL_Rect screenRect();

    void UpdateScreen(SDL_Surface* buffer, SDL_Rect* rect);
    void FlipScreen(bool flipmode);

    const SDL_PixelFormat* GetFormat(void);

    void toggleScalingMode(void);
    void toggleLinearFilter(void);

    bool isWindowed;
    bool isFiltered;
    bool badSignalEffect;
    ScreenScaling scalingMode;
    bool vsync;

    SDL_Surface* m_screen;
    SDL_Rect filterSubrect;

private:
    uint8_t _screenData[SCREEN_WIDTH_VRAM * SCREEN_HEIGHT_VRAM * 4];
    gpu::Framebuffer _screenBuffer;
};


#endif /* SCREEN_H */
