#include "GraphicsResources.h"

#include "FileSystemUtils.h"
#include "Vlogging.h"

#include <pspkernel.h>

// Used to load PNG data
extern "C"
{
    extern unsigned lodepng_decode24(
        unsigned char** out,
        unsigned* w,
        unsigned* h,
        const unsigned char* in,
        size_t insize
    );
    extern unsigned lodepng_decode32(
        unsigned char** out,
        unsigned* w,
        unsigned* h,
        const unsigned char* in,
        size_t insize
    );
}

static SDL_Surface* LoadImage(const char *filename, bool noBlend = true, bool noAlpha = false)
{
    //Temporary storage for the image that's loaded
    SDL_Surface* loadedImage = NULL;
    //The optimized image that will be used
    SDL_Surface* optimizedImage = NULL;

    unsigned char *data;
    unsigned int width, height;

    unsigned char *fileIn;
    size_t length;
    vlog_debug("- Loading image to memory");
    FILESYSTEM_loadAssetToMemory(filename, &fileIn, &length, false);
    if (fileIn == NULL)
    {
        vlog_debug("-! Image file missing");
        SDL_assert(0 && "Image file missing!");
        return NULL;
    }
    if (noAlpha)
    {
        vlog_debug("- Decoding without alpha");
        lodepng_decode24(&data, &width, &height, fileIn, length);
    }
    else
    {
        vlog_debug("- Decoding with alpha");
        lodepng_decode32(&data, &width, &height, fileIn, length);
    }
    vlog_debug("- Freeing memory");
    FILESYSTEM_freeMemory(&fileIn);

    vlog_debug("- Creating surface");
    loadedImage = SDL_CreateRGBSurfaceWithFormatFrom(
        data,
        width,
        height,
        noAlpha ? 24 : 32,
        width * (noAlpha ? 3 : 4),
        noAlpha ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_ABGR8888
    );

    if (loadedImage != NULL)
    {
        optimizedImage = SDL_ConvertSurfaceFormat(
            loadedImage,
            SDL_PIXELFORMAT_ARGB8888,
            0
        );
        SDL_FreeSurface( loadedImage );
        SDL_free(data);
        if (noBlend)
        {
            SDL_SetSurfaceBlendMode(optimizedImage, SDL_BLENDMODE_BLEND);
        }
        return optimizedImage;
    }
    else
    {
        SDL_free(data);
        vlog_error("Image not found: %s", filename);
        SDL_assert(0 && "Image not found! See stderr.");
        return NULL;
    }
}

void GraphicsResources::init(void)
{
    im_tiles =        LoadImage("graphics/tiles.png");
    im_tiles2 =        LoadImage("graphics/tiles2.png");
    im_tiles3 =        LoadImage("graphics/tiles3.png");
    im_entcolours =        LoadImage("graphics/entcolours.png");
    im_sprites =        LoadImage("graphics/sprites.png");
    im_flipsprites =    LoadImage("graphics/flipsprites.png");
    im_bfont =        LoadImage("graphics/font.png");
    im_teleporter =        LoadImage("graphics/teleporter.png");

    im_image0 =        LoadImage("graphics/levelcomplete.png", false);
    im_image1 =        LoadImage("graphics/minimap.png", true, true);
    im_image2 =        LoadImage("graphics/covered.png", true, true);
    im_image3 =        LoadImage("graphics/elephant.png");
    im_image4 =        LoadImage("graphics/gamecomplete.png", false);
    im_image5 =        LoadImage("graphics/fliplevelcomplete.png", false);
    im_image6 =        LoadImage("graphics/flipgamecomplete.png", false);
    im_image7 =        LoadImage("graphics/site.png", false);
    im_image8 =        LoadImage("graphics/site2.png");
    im_image9 =        LoadImage("graphics/site3.png");
    im_image10 =        LoadImage("graphics/ending.png");
    im_image11 =        LoadImage("graphics/site4.png");
    im_image12 =        LoadImage("graphics/minimap.png");
}


void GraphicsResources::destroy(void)
{
#define CLEAR(img) \
    SDL_FreeSurface(img); \
    img = NULL;

    CLEAR(im_tiles);
    CLEAR(im_tiles2);
    CLEAR(im_tiles3);
    CLEAR(im_entcolours);
    CLEAR(im_sprites);
    CLEAR(im_flipsprites);
    CLEAR(im_bfont);
    CLEAR(im_teleporter);

    CLEAR(im_image0);
    CLEAR(im_image1);
    CLEAR(im_image2);
    CLEAR(im_image3);
    CLEAR(im_image4);
    CLEAR(im_image5);
    CLEAR(im_image6);
    CLEAR(im_image7);
    CLEAR(im_image8);
    CLEAR(im_image9);
    CLEAR(im_image10);
    CLEAR(im_image11);
    CLEAR(im_image12);
#undef CLEAR
}