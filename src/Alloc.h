#ifndef ALLOC_H
#define ALLOC_H

#include <SDL2/SDL_surface.h>
#include <cstdint>

#include <SDL2/SDL.h>

template <int W, int H>
struct SurfaceRgba
{
    constexpr static int Width = W;
    constexpr static int Height = H;

    constexpr static int Channels = 4;
    constexpr static int Depth = Channels * 8;
    constexpr static int Pitch = W * Channels;

    uint8_t data[H][W][4];
    SDL_Surface *surface;

    SurfaceRgba()
    : data{0}
    , surface(SDL_CreateRGBSurfaceFrom(
        data,
        W, H,
        Depth, Pitch,
        0x000000FF,
        0x0000FF00,
        0x00FF0000,
        0xFF000000
    ))
    {
    }

    operator SDL_Surface *() {
        return surface;
    }
};

#endif
