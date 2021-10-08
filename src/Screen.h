#ifndef SCREEN_H
#define SCREEN_H

#include <SDL2/SDL.h>
#include <cstdint>

#include "ScreenSettings.h"

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

    void ResizeScreen(int x, int y);
    void ResizeToNearestMultiple(void);
    void GetWindowSize(int* x, int* y);

    void UpdateScreen(SDL_Surface* buffer, SDL_Rect* rect);
    void FlipScreen(bool flipmode);

    const SDL_PixelFormat* GetFormat(void);

    void toggleFullScreen(void);
    void toggleStretchMode(void);
    void toggleLinearFilter(void);
    void toggleVSync(void);

    bool isWindowed;
    bool isFiltered;
    bool badSignalEffect;
    int stretchMode;
    bool vsync;

    SDL_Surface* m_screen;
    SDL_Rect filterSubrect;

private:
    uint8_t _screenData[SCREEN_WIDTH_VRAM * SCREEN_HEIGHT_VRAM * 4];
    uint8_t __attribute__((aligned(16))) _drawList[1024*1024]; // Increase if needed
};


#endif /* SCREEN_H */
