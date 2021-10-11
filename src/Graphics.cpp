#define GRAPHICS_DEFINITION
#include "Graphics.h"

#include <utf8/unchecked.h>

#include "Constants.h"
#include "CustomLevels.h"
#include "Entity.h"
#include "Exit.h"
#include "FileSystemUtils.h"
#include "GraphicsUtil.h"
#include "Map.h"
#include "Music.h"
#include "Screen.h"
#include "UtilityClass.h"
#include "Vlogging.h"

Graphics::Graphics()
: backBuffer()
, menubuffer()
, foregroundBuffer()
, tempBuffer()
, warpbuffer()
, warpbuffer_lerp()
, footerbuffer()
, ghostbuffer()
{
}

void Graphics::init(void)
{
    flipmode = false;
    setRect(tiles_rect, 0,0,8,8);
    setRect(sprites_rect, 0,0,32,32);
    setRect(footerrect, 0, 230, 320, 10);
    setRect(prect, 0, 0, 4, 4);
    setRect(line_rect, 0,0,0,0);
    setRect(tele_rect,0,0,96,96);
    setRect(towerbuffer_rect, 8, 8, 320, 240);


    //We initialise a few things

    linestate = 0;


    trinketcolset = false;

    showcutscenebars = false;
    setbars(0);
    notextoutline = false;

    flipmode = false;
    setflipmode = false;

    //Background inits
    for (int i = 0; i < numstars; i++)
    {
        SDL_Rect s = {(int) (fRandom() * 320), (int) (fRandom() * 240), 2, 2};
        int s2 = 4+(fRandom()*4);
        stars[i] = s;
        starsspeed[i] = s2;
    }

    for (int i = 0; i < numbackboxes; i++)
    {
        SDL_Rect bb;
        int bvx = 0;
        int bvy = 0;
        if(fRandom()*100 > 50)
        {
            bvx = 9 - (fRandom() * 19);
            if (bvx > -6 && bvx < 6) bvx = 6;
            bvx = bvx * 1.5;
            setRect(bb, fRandom() * 320, fRandom() * 240, 32, 12);
        }
        else
        {
            bvy = 9 - (fRandom() * 19);
            if (bvy > -6 && bvy < 6) bvy = 6;
            bvy = bvy * 1.5;
            setRect(bb, fRandom() * 320, fRandom() * 240, 12, 32) ;
        }
        float bint = 0.5 + ((fRandom() * 100) / 200);
        backboxes[i] = bb;
        backboxvx[i] = bvx;
        backboxvy[i] = bvy;
        backboxint[i] = bint;
    }
    backoffset = 0;
    backgrounddrawn = false;

    warpskip = 0;
    warpfcol = 0x000000;
    warpbcol = 0x000000;

    spcol = 0;
    spcoldel = 0;
    rcol = 0;

    crewframe = 0;
    crewframedelay = 4;
    menuoffset = 0;
    oldmenuoffset = 0;
    resumegamemode = false;

    //Fading stuff
    SDL_memset(fadebars, 0, sizeof(fadebars));

    setfade(0);
    fademode = 0;
    ingame_fademode = 0;

    // initialize everything else to zero
    ct = colourTransform();
    foregrounddrawn = false;
    backgrounddrawn = false;
    m = 0;
    linedelay = 0;
    screenbuffer = NULL;
    towerbg = TowerBG();
    titlebg = TowerBG();
    trinketr = 0;
    trinketg = 0;
    trinketb = 0;
    warprect = SDL_Rect();

    translucentroomname = false;

    alpha = 1.0f;

    screenshake_x = 0;
    screenshake_y = 0;

    col_crewred = 0x00000000;
    col_crewyellow = 0x00000000;
    col_crewgreen = 0x00000000;
    col_crewcyan = 0x00000000;
    col_crewblue = 0x00000000;
    col_crewpurple = 0x00000000;
    col_crewinactive = 0x00000000;
    col_clock = 0x00000000;
    col_trinket = 0x00000000;
    col_tr = 0;
    col_tg = 0;
    col_tb = 0;

    kludgeswnlinewidth = false;

#ifndef NO_CUSTOM_LEVELS
    tiles1_mounted = false;
    tiles2_mounted = false;
    minimap_mounted = false;
#endif

    SDL_zeroa(error);
    SDL_zeroa(error_title);
}

void Graphics::destroy(void)
{
    #define CLEAR_ARRAY(name) \
        for (size_t i = 0; i < name.size(); i += 1) \
        { \
            SDL_FreeSurface(name[i]); \
        } \
        name.clear();

    CLEAR_ARRAY(tiles)
    CLEAR_ARRAY(tiles2)
    CLEAR_ARRAY(tiles3)
    CLEAR_ARRAY(entcolours)
    CLEAR_ARRAY(sprites)
    CLEAR_ARRAY(flipsprites)
    CLEAR_ARRAY(tele)
    CLEAR_ARRAY(bfont)
    CLEAR_ARRAY(flipbfont)

    #undef CLEAR_ARRAY
}

void Graphics::create_buffers(const SDL_PixelFormat* fmt)
{
    #define CREATE_SURFACE(w, h) \
        SDL_CreateRGBSurface( \
            SDL_SWSURFACE, \
            w, h, \
            fmt->BitsPerPixel, \
            fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask \
        )
    SDL_SetSurfaceBlendMode(backBuffer, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(footerbuffer, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceAlphaMod(footerbuffer, 127);
    FillRect(footerbuffer, SDL_MapRGB(fmt, 0, 0, 0));
    SDL_SetSurfaceBlendMode(ghostbuffer, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceAlphaMod(ghostbuffer, 127);
    SDL_SetSurfaceBlendMode(foregroundBuffer, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceBlendMode(menubuffer, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(warpbuffer, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(warpbuffer_lerp, SDL_BLENDMODE_NONE);

    towerbg.buffer =  CREATE_SURFACE(320 + 16, 240 + 16);
    SDL_SetSurfaceBlendMode(towerbg.buffer, SDL_BLENDMODE_NONE);

    towerbg.buffer_lerp = CREATE_SURFACE(320 + 16, 240 + 16);
    SDL_SetSurfaceBlendMode(towerbg.buffer_lerp, SDL_BLENDMODE_NONE);

    titlebg.buffer = CREATE_SURFACE(320 + 16, 240 + 16);
    SDL_SetSurfaceBlendMode(titlebg.buffer, SDL_BLENDMODE_NONE);

    titlebg.buffer_lerp = CREATE_SURFACE(320 + 16, 240 + 16);
    SDL_SetSurfaceBlendMode(titlebg.buffer_lerp, SDL_BLENDMODE_NONE);

    SDL_SetSurfaceBlendMode(tempBuffer, SDL_BLENDMODE_NONE);

    #undef CREATE_SURFACE
}

void Graphics::destroy_buffers(void)
{
#define FREE_SURFACE(SURFACE) \
    SDL_FreeSurface(SURFACE); \
    SURFACE = NULL;

    FREE_SURFACE(towerbg.buffer)
    FREE_SURFACE(towerbg.buffer_lerp)
    FREE_SURFACE(titlebg.buffer)
    FREE_SURFACE(titlebg.buffer_lerp)

#undef FREE_SURFACE
}

int Graphics::font_idx(uint32_t ch)
{
    if (font_positions.size() > 0)
    {
        std::map<int, int>::iterator iter = font_positions.find(ch);
        if (iter == font_positions.end())
        {
            iter = font_positions.find('?');
            if (iter == font_positions.end())
            {
                WHINE_ONCE("font.txt missing fallback character!");
                return -1;
            }
        }
        return iter->second;
    }
    else
    {
        return ch;
    }
}

void Graphics::drawspritesetcol(int x, int y, int t, int c)
{
    if (!INBOUNDS_VEC(t, sprites))
    {
        return;
    }
    SDL_Rect rect;
    setRect(rect,x,y,sprites_rect.w,sprites_rect.h);
    setcol(c);

    BlitSurfaceColoured(sprites[t],NULL,backBuffer, &rect, ct);
}

void Graphics::updatetitlecolours(void)
{
    setcol(15);
    col_crewred = ct.colour;
    setcol(14);
    col_crewyellow = ct.colour;
    setcol(13);
    col_crewgreen = ct.colour;
    setcol(0);
    col_crewcyan = ct.colour;
    setcol(16);
    col_crewblue = ct.colour;
    setcol(20);
    col_crewpurple = ct.colour;
    setcol(19);
    col_crewinactive = ct.colour;

    setcol(18);
    col_clock = ct.colour;
    setcol(18);
    col_trinket = ct.colour;
}

#define PROCESS_TILESHEET_CHECK_ERROR(tilesheet, tile_square) \
    if (grphx.im_##tilesheet == NULL) \
    { \
        /* We have already asserted; just no-op. */ \
    } \
    else if (grphx.im_##tilesheet->w % tile_square != 0 \
    || grphx.im_##tilesheet->h % tile_square != 0) \
    { \
        static const char error_fmt[] = "%s.png dimensions not exact multiples of %i!"; \
        static const char error_title_fmt[] = "Error with %s.png"; \
        \
        SDL_snprintf(error, sizeof(error), error_fmt, #tilesheet, tile_square); \
        SDL_snprintf(error_title, sizeof(error_title), error_title_fmt, #tilesheet); \
        \
        vlog_error(error); \
        \
        return false; \
    }

#define PROCESS_TILESHEET_RENAME(tilesheet, vector, tile_square, extra_code) \
    PROCESS_TILESHEET_CHECK_ERROR(tilesheet, tile_square) \
    \
    else \
    { \
        int j; \
        for (j = 0; j < grphx.im_##tilesheet->h / tile_square; ++j) \
        { \
            int i; \
            for (i = 0; i < grphx.im_##tilesheet->w / tile_square; ++i) \
            { \
                SDL_Surface* temp = GetSubSurface( \
                    grphx.im_##tilesheet, \
                    i * tile_square, j * tile_square, \
                    tile_square, tile_square \
                ); \
                vector.push_back(temp); \
                \
                extra_code \
            } \
        } \
        \
        SDL_FreeSurface(grphx.im_##tilesheet); \
        grphx.im_##tilesheet = NULL; \
    }

#define PROCESS_TILESHEET(tilesheet, tile_square, extra_code) \
    PROCESS_TILESHEET_RENAME(tilesheet, tilesheet, tile_square, extra_code)

bool Graphics::Makebfont(void)
{
    PROCESS_TILESHEET(bfont, 8,
    {
        SDL_Surface* TempFlipped = FlipSurfaceVerticle(temp);
        flipbfont.push_back(TempFlipped);
    })

    unsigned char* charmap;
    size_t length;
    FILESYSTEM_loadAssetToMemory("graphics/font.txt", &charmap, &length, false);
    if (charmap != NULL)
    {
        unsigned char* current = charmap;
        unsigned char* end = charmap + length;
        int pos = 0;
        while (current != end)
        {
            int codepoint = utf8::unchecked::next(current);
            font_positions[codepoint] = pos;
            ++pos;
        }
        FILESYSTEM_freeMemory(&charmap);
    }
    else
    {
        font_positions.clear();
    }

    return true;
}

int Graphics::bfontlen(uint32_t ch)
{
    if (ch < 32)
    {
        return 6;
    }
    else
    {
        return 8;
    }
}

bool Graphics::MakeTileArray(void)
{
    PROCESS_TILESHEET(tiles, 8, {})
    PROCESS_TILESHEET(tiles2, 8, {})
    PROCESS_TILESHEET(tiles3, 8, {})
    PROCESS_TILESHEET(entcolours, 8, {})

    return true;
}

bool Graphics::maketelearray(void)
{
    PROCESS_TILESHEET_RENAME(teleporter, tele, 96, {})

    return true;
}

bool Graphics::MakeSpriteArray(void)
{
    PROCESS_TILESHEET(sprites, 32, {})
    PROCESS_TILESHEET(flipsprites, 32, {})

    return true;
}

#undef PROCESS_TILESHEET
#undef PROCESS_TILESHEET_RENAME
#undef PROCESS_TILESHEET_CHECK_ERROR


void Graphics::map_tab(int opt, const std::string& text, bool selected /*= false*/)
{
    int x = opt*80 + 40 - len(text)/2;
    if (selected)
    {
        Print(x-8, 220, "[" + text + "]", 196, 196, 255 - help.glow);
    }
    else
    {
        Print(x, 220, text, 64, 64, 64);
    }
}

void Graphics::map_option(int opt, int num_opts, const std::string& text, bool selected /*= false*/)
{
    int x = 80 + opt*32;
    int y = 136; // start from middle of menu

    int yoff = -(num_opts * 12) / 2; // could be simplified to -num_opts * 6, this conveys my intent better though
    yoff += opt * 12;

    if (flipmode)
    {
        y -= yoff; // going down, which in Flip Mode means going up
        y -= 40;
    }
    else
    {
        y += yoff; // going up
    }

    if (selected)
    {
        std::string text_upper(text);
        for (size_t i = 0; i < text_upper.length(); i++)
        {
            text_upper[i] = SDL_toupper(text_upper[i]);
        }
        Print(x - 16, y, "[ " + text_upper + " ]", 196, 196, 255 - help.glow);
    }
    else
    {
        Print(x, y, text, 96, 96, 96);
    }
}

static void print_char(
    SDL_Surface* const buffer,
    SDL_Surface* const font,
    const int x,
    const int y,
    const int scale,
    colourTransform& ct
) {
    SDL_Rect font_rect = {x, y, 8*scale, 8*scale};
    SDL_Surface* surface;

    if (scale > 1)
    {
        surface = ScaleSurface(font, 8 * scale, 8 * scale);
        if (surface == NULL)
        {
            return;
        }
    }
    else
    {
        surface = font;
    }

    BlitSurfaceColoured(surface, NULL, buffer, &font_rect, ct);

    if (scale > 1)
    {
        SDL_FreeSurface(surface);
    }
}

void Graphics::do_print(
    const int x,
    const int y,
    const std::string& text,
    int r,
    int g,
    int b,
    int a,
    const int scale
) {
    std::vector<SDL_Surface*>& font = flipmode ? flipbfont : bfont;

    int position = 0;
    std::string::const_iterator iter = text.begin();

    r = clamp(r, 0, 255);
    g = clamp(g, 0, 255);
    b = clamp(b, 0, 255);
    a = clamp(a, 0, 255);

    ct.colour = getRGBA(r, g, b, a);

    while (iter != text.end())
    {
        const uint32_t character = utf8::unchecked::next(iter);
        const int idx = font_idx(character);

        if (INBOUNDS_VEC(idx, font))
        {
            print_char(backBuffer, font[idx], x + position, y, scale, ct);
        }

        position += bfontlen(character) * scale;
    }
}

void Graphics::Print( int _x, int _y, const std::string& _s, int r, int g, int b, bool cen /*= false*/ ) {
    return PrintAlpha(_x,_y,_s,r,g,b,255,cen);
}

void Graphics::PrintAlpha( int _x, int _y, const std::string& _s, int r, int g, int b, int a, bool cen /*= false*/ )
{
    if (cen)
        _x = ((160 ) - ((len(_s)) / 2));

    return do_print(_x, _y, _s, r, g, b, a, 1);
}

bool Graphics::next_wrap(
    size_t* start,
    size_t* len,
    const char* str,
    const int maxwidth
) {
    /* This function is UTF-8 aware. But start/len still are bytes. */
    size_t idx = 0;
    size_t lenfromlastspace = 0;
    size_t lastspace = 0;
    int linewidth = 0;
    *len = 0;

    if (str[idx] == '\0')
    {
        return false;
    }

    while (true)
    {
        /* FIXME: This only checks one byte, not multiple! */
        if ((str[idx] & 0xC0) == 0x80)
        {
            /* Skip continuation byte. */
            goto next;
        }

        linewidth += bfontlen(str[idx]);

        switch (str[idx])
        {
        case ' ':
            lenfromlastspace = idx;
            lastspace = *start;
            break;
        case '\n':
            *start += 1;
            VVV_fallthrough;
        case '\0':
            return true;
        }

        if (linewidth > maxwidth)
        {
            if (lenfromlastspace != 0)
            {
                *len = lenfromlastspace;
                *start = lastspace + 1;
            }
            return true;
        }

next:
        idx += 1;
        *start += 1;
        *len += 1;
    }
}

bool Graphics::next_wrap_s(
    char buffer[],
    const size_t buffer_size,
    size_t* start,
    const char* str,
    const int maxwidth
) {
    size_t len = 0;
    const size_t prev_start = *start;

    const bool retval = next_wrap(start, &len, &str[*start], maxwidth);

    if (retval)
    {
        /* Like next_split_s(), don't use SDL_strlcpy() here. */
        const size_t length = VVV_min(buffer_size - 1, len);
        SDL_memcpy(buffer, &str[prev_start], length);
        buffer[length] = '\0';
    }

    return retval;
}

void Graphics::PrintWrap(
    const int x,
    int y,
    const char* str,
    const int r,
    const int g,
    const int b,
    const bool cen,
    const int linespacing,
    const int maxwidth
) {
    /* Screen width is 320 pixels. The shortest a char can be is 6 pixels wide.
     * 320 / 6 is 54, rounded up. 4 bytes per char. */
    char buffer[54*4 + 1];
    size_t start = 0;

    if (flipmode)
    {
        /* Correct for the height of the resulting print. */
        size_t len = 0;
        while (next_wrap(&start, &len, &str[start], maxwidth))
        {
            y += linespacing;
        }
        y -= linespacing;
        start = 0;
    }

    while (next_wrap_s(buffer, sizeof(buffer), &start, str, maxwidth))
    {
        Print(x, y, buffer, r, g, b, cen);

        if (flipmode)
        {
            y -= linespacing;
        }
        else
        {
            y += linespacing;
        }
    }
}


void Graphics::bigprint(  int _x, int _y, const std::string& _s, int r, int g, int b, bool cen, int sc )
{
    if (cen)
    {
        _x = VVV_max(160 - (int((len(_s)/ 2.0)*sc)), 0 );
    }

    return do_print(_x, _y, _s, r, g, b, 255, sc);
}

void Graphics::bigbprint(int x, int y, const std::string& s, int r, int g, int b, bool cen, int sc)
{
    if (!notextoutline)
    {
        bigprint(x, y - sc, s, 0, 0, 0, cen, sc);
        if (cen)
        {
            int x_cen = VVV_max(160 - (len(s) / 2) * sc, 0);
            bigprint(x_cen - sc, y, s, 0, 0, 0, false, sc);
            bigprint(x_cen + sc, y, s, 0, 0, 0, false, sc);
        }
        else
        {
            bigprint(x - sc, y, s, 0, 0, 0, cen, sc);
            bigprint(x + sc, y, s, 0, 0, 0, cen, sc);
        }
        bigprint(x, y + sc, s, 0, 0, 0, cen, sc);
    }

    bigprint(x, y, s, r, g, b, cen, sc);
}

int Graphics::len(const std::string& t)
{
    int bfontpos = 0;
    std::string::const_iterator iter = t.begin();
    while (iter != t.end()) {
        int cur = utf8::unchecked::next(iter);
        bfontpos += bfontlen(cur);
    }
    return bfontpos;
}

void Graphics::bprint( int x, int y, const std::string& t, int r, int g, int b, bool cen /*= false*/ ) {
    bprintalpha(x,y,t,r,g,b,255,cen);
}

void Graphics::bprintalpha( int x, int y, const std::string& t, int r, int g, int b, int a, bool cen /*= false*/ )
{
    if (!notextoutline)
    {
        PrintAlpha(x, y - 1, t, 0, 0, 0, a, cen);
        if (cen)
        {
            const int x_cen = 160 - len(t)/2;
            PrintAlpha(x_cen - 1, y, t, 0, 0, 0, a, false);
            PrintAlpha(x_cen + 1, y, t, 0, 0, 0, a, false);
        }
        else
        {
            PrintAlpha(x  -1, y, t, 0, 0, 0, a, cen);
            PrintAlpha(x  +1, y, t, 0, 0, 0, a, cen);
        }
        PrintAlpha(x, y+1, t, 0, 0, 0, a, cen);
    }

    PrintAlpha(x, y, t, r, g, b, a, cen);
}

void Graphics::printcrewname( int x, int y, int t )
{
    //Print the name of crew member t in the right colour
    switch(t)
    {
    case 0:
        Print(x, y, "Viridian", 16, 240, 240,false );
        break;
    case 1:
        Print(x, y, "Violet", 240, 16, 240,false);
        break;
    case 2:
        Print(x, y, "Vitellary", 240, 240, 16,false);
        break;
    case 3:
        Print(x, y, "Vermilion", 240, 16, 16,false);
        break;
    case 4:
        Print(x, y, "Verdigris", 16, 240, 16,false);
        break;
    case 5:
        Print(x, y, "Victoria", 16, 16, 240,false);
        break;
    }
}

void Graphics::printcrewnamedark( int x, int y, int t )
{
    //Print the name of crew member t as above, but in black and white
    switch(t)
    {
    case 0:
        Print(x, y, "Viridian", 128,128,128,false);
        break;
    case 1:
        Print(x, y, "Violet", 128,128,128,false);
        break;
    case 2:
        Print(x, y, "Vitellary", 128,128,128,false);
        break;
    case 3:
        Print(x, y, "Vermilion", 128,128,128,false);
        break;
    case 4:
        Print(x, y, "Verdigris", 128,128,128,false);
        break;
    case 5:
        Print(x, y, "Victoria", 128,128,128,false);
        break;
    }
}

void Graphics::printcrewnamestatus( int x, int y, int t )
{
    //Print the status of crew member t in the right colour
    switch(t)
    {
    case 0:
        Print(x, y, "(that's you!)", 12, 140, 140,false);
        break;
    case 1:
        Print(x, y, "Rescued!", 140, 12, 140,false);
        break;
    case 2:
        Print(x, y, "Rescued!", 140, 140, 12,false);
        break;
    case 3:
        Print(x, y, "Rescued!", 140, 12, 12,false);
        break;
    case 4:
        Print(x, y, "Rescued!", 12, 140, 12,false);
        break;
    case 5:
        Print(x, y, "Rescued!", 12, 12, 140,false);
        break;
    }
}

void Graphics::drawsprite( int x, int y, int t, int r, int g,  int b )
{
    if (!INBOUNDS_VEC(t, sprites))
    {
        WHINE_ONCE("drawsprite() out-of-bounds!");
        return;
    }

    SDL_Rect rect = {x, y, sprites_rect.w, sprites_rect.h};
    setcolreal(getRGB(r,g,b));
    BlitSurfaceColoured(sprites[t], NULL, backBuffer, &rect, ct);
}

void Graphics::drawsprite(int x, int y, int t, Uint32 c)
{
    if (!INBOUNDS_VEC(t, sprites))
    {
        WHINE_ONCE("drawsprite() out-of-bounds!");
        return;
    }

    SDL_Rect rect = {x, y, sprites_rect.w, sprites_rect.h};
    setcolreal(c);
    BlitSurfaceColoured(sprites[t], NULL, backBuffer, &rect, ct);
}

#ifndef NO_CUSTOM_LEVELS
bool Graphics::shouldrecoloroneway(const int tilenum, const bool mounted)
{
    return (tilenum >= 14 && tilenum <= 17
    && (!mounted
    || cl.onewaycol_override));
}
#endif

void Graphics::drawtile( int x, int y, int t )
{
    if (!INBOUNDS_VEC(t, tiles))
    {
        WHINE_ONCE("drawtile() out-of-bounds!");
        return;
    }

    SDL_Rect rect = {x, y, tiles_rect.w, tiles_rect.h};

#if !defined(NO_CUSTOM_LEVELS)
    if (shouldrecoloroneway(t, tiles1_mounted))
    {
        colourTransform thect = {cl.getonewaycol()};
        BlitSurfaceTinted(tiles[t], NULL, backBuffer, &rect, thect);
    }
    else
#endif
    {
        BlitSurfaceStandard(tiles[t], NULL, backBuffer, &rect);
    }
}


void Graphics::drawtile2( int x, int y, int t )
{
    if (!INBOUNDS_VEC(t, tiles2))
    {
        WHINE_ONCE("drawtile2() out-of-bounds!");
        return;
    }

    SDL_Rect rect = {x, y, tiles_rect.w, tiles_rect.h};

#if !defined(NO_CUSTOM_LEVELS)
    if (shouldrecoloroneway(t, tiles2_mounted))
    {
        colourTransform thect = {cl.getonewaycol()};
        BlitSurfaceTinted(tiles2[t], NULL, backBuffer, &rect, thect);
    }
    else
#endif
    {
        BlitSurfaceStandard(tiles2[t], NULL, backBuffer, &rect);
    }
}



void Graphics::drawtile3( int x, int y, int t, int off, int height_subtract /*= 0*/ )
{
    t += off * 30;
    if (!INBOUNDS_VEC(t, tiles3))
    {
        WHINE_ONCE("drawtile3() out-of-bounds!");
        return;
    }
    SDL_Rect src_rect = { 0, 0, tiles_rect.w, tiles_rect.h - height_subtract };
    SDL_Rect rect = {x, y, tiles_rect.w, tiles_rect.h};
    BlitSurfaceStandard(tiles3[t], &src_rect, backBuffer, &rect);
}

void Graphics::drawtowertile( int x, int y, int t )
{
    if (!INBOUNDS_VEC(t, tiles2))
    {
        WHINE_ONCE("drawtowertile() out-of-bounds!");
        return;
    }
    x += 8;
    y += 8;
    SDL_Rect rect = {x, y, tiles_rect.w, tiles_rect.h};
    BlitSurfaceStandard(tiles2[t], NULL, warpbuffer, &rect);
}


void Graphics::drawtowertile3( int x, int y, int t, TowerBG& bg_obj )
{
    t += bg_obj.colstate*30;
    if (!INBOUNDS_VEC(t, tiles3))
    {
        WHINE_ONCE("drawtowertile3() out-of-bounds!");
        return;
    }
    x += 8;
    y += 8;
    SDL_Rect rect = {x, y, tiles_rect.w, tiles_rect.h};
    BlitSurfaceStandard(tiles3[t], NULL, bg_obj.buffer, &rect);
}

void Graphics::drawgui(void)
{
    int text_sign;
    int crew_yp;
    int crew_sprite;
    size_t i;

    if (flipmode)
    {
        text_sign = -1;
        crew_yp = 64 + 48 + 4;
        crew_sprite = 6;
    }
    else
    {
        text_sign = 1;
        crew_yp = 64 + 32 + 4;
        crew_sprite = 0;
    }

    //Draw all the textboxes to the screen
    for (i = 0; i<textboxes.size(); i++)
    {
        int text_yoff;
        int yp;
        bool opaque;
        if (flipmode)
        {
            text_yoff = textboxes[i].lines.size() * 8;
        }
        else
        {
            text_yoff = 8;
        }

        yp = textboxes[i].yp;
        if (flipmode && textboxes[i].flipme)
        {
            yp = SCREEN_HEIGHT_PIXELS - yp - 8 * (textboxes[i].lines.size() + 2);
        }

        if (textboxes[i].r == 0 && textboxes[i].g == 0 && textboxes[i].b == 0)
        {
            size_t j;
            for (j = 0; j < textboxes[i].lines.size(); j++)
            {
                bprint(textboxes[i].xp + 8, yp + text_yoff + text_sign * (j * 8), textboxes[i].lines[j], 196, 196, 255 - help.glow);
            }
        }
        else
        {
            const float tl_lerp = lerp(textboxes[i].prev_tl, textboxes[i].tl);
            const int r = textboxes[i].r * tl_lerp;
            const int g = textboxes[i].g * tl_lerp;
            const int b = textboxes[i].b * tl_lerp;
            size_t j;

            drawtextbox(textboxes[i].xp, yp, textboxes[i].w/8, textboxes[i].h/8, r, g, b);

            for (j = 0; j < textboxes[i].lines.size(); j++)
            {
                Print(textboxes[i].xp + 8, yp + text_yoff + text_sign * (j * 8), textboxes[i].lines[j], r, g, b);
            }
        }

        opaque = textboxes[i].tl >= 1.0;

        if (!opaque)
        {
            continue;
        }

        if (textboxes[i].yp == 12 && textboxes[i].r == 165)
        {
            if (flipmode)
            {
                drawimage(5, 0, 180, true);
            }
            else
            {
                drawimage(0, 0, 12, true);
            }
        }
        else if (textboxes[i].yp == 12 && textboxes[i].g == 165)
        {
            if (flipmode)
            {
                drawimage(6, 0, 180, true);
            }
            else
            {
                drawimage(4, 0, 12, true);
            }
        }
        if (textboxes[i].r == 175 && textboxes[i].g == 175)
        {
            //purple guy
            drawsprite(80 - 6, crew_yp, crew_sprite, 220- help.glow/4 - textboxes[i].rand, 120- help.glow/4, 210 - help.glow/4);
        }
        else if (textboxes[i].r == 175 && textboxes[i].b == 175)
        {
            //red guy
            drawsprite(80 - 6, crew_yp, crew_sprite, 255 - help.glow/8, 70 - help.glow/4, 70 - help.glow / 4);
        }
        else if (textboxes[i].r == 175)
        {
            //green guy
            drawsprite(80 - 6, crew_yp, crew_sprite, 120 - help.glow / 4 - textboxes[i].rand, 220 - help.glow / 4, 120 - help.glow / 4);
        }
        else if (textboxes[i].g == 175)
        {
            //yellow guy
            drawsprite(80 - 6, crew_yp, crew_sprite, 220- help.glow/4 - textboxes[i].rand, 210 - help.glow/4, 120- help.glow/4);
        }
        else if (textboxes[i].b == 175)
        {
            //blue guy
            drawsprite(80 - 6, crew_yp, crew_sprite, 75, 75, 255- help.glow/4 - textboxes[i].rand);
        }
    }
}

void Graphics::updatetextboxes(void)
{
    for (size_t i = 0; i < textboxes.size(); i++)
    {
        textboxes[i].update();

        if (textboxes[i].tm == 2 && textboxes[i].tl <= 0.5)
        {
            textboxes.erase(textboxes.begin() + i);
            i--;
            continue;
        }

        if (textboxes[i].tl >= 1.0f
        && ((textboxes[i].r == 175 && textboxes[i].g == 175)
        || textboxes[i].r == 175
        || textboxes[i].g == 175
        || textboxes[i].b == 175)
        && (textboxes[i].r != 175 || textboxes[i].b != 175))
        {
            textboxes[i].rand = fRandom() * 20;
        }
    }
}

void Graphics::drawimagecol( int t, int xp, int yp, bool cent/*= false*/ )
{
    if (!INBOUNDS_VEC(t, images))
    {
        return;
    }
    SDL_Rect trect;

    point tpoint;
    if (cent)
    {
        tpoint.x = 160 - int(images[t]->w / 2);
        tpoint.y = yp;
        trect.x = tpoint.x ;
        trect.y = tpoint.y;
        trect.w = images[t]->w;
        trect.h= images[t]->h;
        BlitSurfaceColoured(images[t], NULL, backBuffer, &trect, ct);

    }
    else
    {
        trect.x = xp;
        trect.y = yp;
        trect.w = images[t]->w;
        trect.h = images[t]->h;
        BlitSurfaceColoured(images[t], NULL, backBuffer, &trect, ct);

    }
}

void Graphics::drawimage( int t, int xp, int yp, bool cent/*=false*/ )
{
    if (!INBOUNDS_VEC(t, images))
    {
        return;
    }

    SDL_Rect trect;
    if (cent)
    {
        trect.x = 160 - int(images[t]->w / 2);
        trect.y = yp;
        trect.w = images[t]->w;
        trect.h = images[t]->h;
        BlitSurfaceStandard(images[t], NULL, backBuffer, &trect);
    }
    else
    {

        trect.x = xp;
        trect.y = yp;
        trect.w = images[t]->w;
        trect.h= images[t]->h;

        BlitSurfaceStandard(images[t], NULL, backBuffer, &trect);
    }
}

void Graphics::drawpartimage( int t, int xp, int yp, int wp, int hp)
{
  if (!INBOUNDS_VEC(t, images))
  {
    return;
  }

  SDL_Rect trect;

  trect.x = xp;
  trect.y = yp;
  trect.w = wp;
  trect.h= hp;

  SDL_Rect trect2;

  trect2.x = 0;
  trect2.y = 0;
  trect2.w = wp;
  trect2.h= hp;

  BlitSurfaceStandard(images[t], &trect2, backBuffer, &trect);
}

void Graphics::cutscenebars(void)
{
    int usethispos = lerp(oldcutscenebarspos, cutscenebarspos);
    if (showcutscenebars)
    {
        FillRect(backBuffer, 0, 0, usethispos, 16, 0x000000);
        FillRect(backBuffer, 360-usethispos, 224, usethispos, 16, 0x000000);
    }
    else if (cutscenebarspos > 0) //disappearing
    {
        //draw
        FillRect(backBuffer, 0, 0, usethispos, 16, 0x000000);
        FillRect(backBuffer, 360-usethispos, 224, usethispos, 16, 0x000000);
    }
}

void Graphics::cutscenebarstimer(void)
{
    oldcutscenebarspos = cutscenebarspos;
    if (showcutscenebars)
    {
        cutscenebarspos += 25;
        cutscenebarspos = VVV_min(cutscenebarspos, 361);
    }
    else if (cutscenebarspos > 0)
    {
        //disappearing
        cutscenebarspos -= 25;
        cutscenebarspos = VVV_max(cutscenebarspos, 0);
    }
}

void Graphics::setbars(const int position)
{
    cutscenebarspos = position;
    oldcutscenebarspos = position;
}

void Graphics::drawcrewman( int x, int y, int t, bool act, bool noshift /*=false*/ )
{
    if (!act)
    {
        if (noshift)
        {
            if (flipmode)
            {
                drawsprite(x, y, 14, col_crewinactive);
            }
            else
            {
                drawsprite(x, y, 12, col_crewinactive);
            }
        }
        else
        {
            if (flipmode)
            {
                drawsprite(x - 8, y, 14, col_crewinactive);
            }
            else
            {
                drawsprite(x - 8, y, 12, col_crewinactive);
            }
        }
    }
    else
    {
        if (flipmode) crewframe += 6;

        switch(t)
        {
        case 0:
            drawsprite(x, y, crewframe, col_crewcyan);
            break;
        case 1:
            drawsprite(x, y, crewframe, col_crewpurple);
            break;
        case 2:
            drawsprite(x, y, crewframe, col_crewyellow);
            break;
        case 3:
            drawsprite(x, y, crewframe, col_crewred);
            break;
        case 4:
            drawsprite(x, y, crewframe, col_crewgreen);
            break;
        case 5:
            drawsprite(x, y, crewframe, col_crewblue);
            break;
        }

        if (flipmode) crewframe -= 6;
    }
}

void Graphics::drawpixeltextbox(
    const int x,
    const int y,
    const int w,
    const int h,
    const int r,
    const int g,
    const int b
) {
    int k;

    FillRect(backBuffer, x, y, w, h, r/6, g/6, b/6);

    /* Horizontal tiles */
    for (k = 0; k < w/8 - 2; ++k)
    {
        drawcoloredtile(x + 8 + k*8, y, 41, r, g, b);
        drawcoloredtile(x + 8 + k*8, y + h - 8, 46, r, g, b);
    }

    if (w % 8 != 0)
    {
        /* Fill in horizontal gap */
        drawcoloredtile(x + w - 16, y, 41, r, g, b);
        drawcoloredtile(x + w - 16, y + h - 8, 46, r, g, b);
    }

    /* Vertical tiles */
    for (k = 0; k < h/8 - 2; ++k)
    {
        drawcoloredtile(x, y + 8 + k*8, 43, r, g, b);
        drawcoloredtile(x + w - 8, y + 8 + k*8, 44, r, g, b);
    }

    if (h % 8 != 0)
    {
        /* Fill in vertical gap */
        drawcoloredtile(x, y + h - 16, 43, r, g, b);
        drawcoloredtile(x + w - 8, y + h - 16, 44, r, g, b);
    }

    /* Corners */
    drawcoloredtile(x, y, 40, r, g, b);
    drawcoloredtile(x + w - 8, y, 42, r, g, b);
    drawcoloredtile(x, y + h - 8, 45, r, g, b);
    drawcoloredtile(x + w - 8, y + h - 8, 47, r, g, b);
}

void Graphics::drawtextbox(
    const int x,
    const int y,
    const int w,
    const int h,
    const int r,
    const int g,
    const int b
) {
    return drawpixeltextbox(x, y, w*8, h*8, r, g, b);
}

void Graphics::textboxactive(void)
{
    //Remove all but the most recent textbox
    for (int i = 0; i < (int) textboxes.size(); i++)
    {
        if (m != i) textboxes[i].remove();
    }
}

void Graphics::textboxremovefast(void)
{
    //Remove all textboxes
    for (size_t i = 0; i < textboxes.size(); i++)
    {
        textboxes[i].removefast();
    }
}

void Graphics::textboxremove(void)
{
    //Remove all textboxes
    for (size_t i = 0; i < textboxes.size(); i++)
    {
        textboxes[i].remove();
    }
}

void Graphics::textboxtimer( int t )
{
    if (!INBOUNDS_VEC(m, textboxes))
    {
        vlog_error("textboxtimer() out-of-bounds!");
        return;
    }

    textboxes[m].timer=t;
}

void Graphics::addline( const std::string& t )
{
    if (!INBOUNDS_VEC(m, textboxes))
    {
        vlog_error("addline() out-of-bounds!");
        return;
    }

    textboxes[m].addline(t);
}

void Graphics::textboxadjust(void)
{
    if (!INBOUNDS_VEC(m, textboxes))
    {
        vlog_error("textboxadjust() out-of-bounds!");
        return;
    }

    textboxes[m].adjust();
}


void Graphics::createtextboxreal(
    const std::string& t,
    int xp,
    int yp,
    int r,
    int g,
    int b,
    bool flipme
) {
    m = textboxes.size();

    if(m<20)
    {
        textboxclass text;
        text.lines.push_back(t);
        text.xp = xp;
        int length = utf8::unchecked::distance(t.begin(), t.end());
        if (xp == -1) text.xp = 160 - (((length / 2) + 1) * 8);
        text.yp = yp;
        text.initcol(r, g, b);
        text.flipme = flipme;
        text.resize();
        textboxes.push_back(text);
    }
}

void Graphics::createtextbox(
    const std::string& t,
    int xp,
    int yp,
    int r,
    int g,
    int b
) {
    createtextboxreal(t, xp, yp, r, g, b, false);
}

void Graphics::createtextboxflipme(
    const std::string& t,
    int xp,
    int yp,
    int r,
    int g,
    int b
) {
    createtextboxreal(t, xp, yp, r, g, b, true);
}

void Graphics::drawfade(void)
{
    int usethisamount = lerp(oldfadeamount, fadeamount);
    if ((fademode == 1)||(fademode == 4))
    {
        ClearSurface(backBuffer);
    }
    else if(fademode==3)
    {
        for (size_t i = 0; i < SDL_arraysize(fadebars); i++)
        {
            FillRect(backBuffer, fadebars[i], i * 16, usethisamount, 16, 0x000000 );
        }
    }
    else if(fademode==5 )
    {
        for (size_t i = 0; i < SDL_arraysize(fadebars); i++)
        {
            FillRect(backBuffer, fadebars[i]-usethisamount, i * 16, 500, 16, 0x000000 );
        }
    }

}

void Graphics::processfade(void)
{
    oldfadeamount = fadeamount;
    if (fademode > 1)
    {
        if (fademode == 2)
        {
            //prepare fade out
            for (size_t i = 0; i < SDL_arraysize(fadebars); i++)
            {
                fadebars[i] = -int(fRandom() * 12) * 8;
            }
            setfade(0);
            fademode = 3;
        }
        else if (fademode == 3)
        {
            fadeamount += 24;
            if (fadeamount > 416)
            {
                fademode = 1; //faded
            }
        }
        else if (fademode == 4)
        {
            //prepare fade in
            for (size_t i = 0; i < SDL_arraysize(fadebars); i++)
            {
                fadebars[i] = 320 + int(fRandom() * 12) * 8;
            }
            setfade(416);
            fademode = 5;
        }
        else if (fademode == 5)
        {
            fadeamount -= 24;
            if (fadeamount <= 0)
            {
                fademode = 0; //normal
            }
        }
    }
}

void Graphics::setfade(const int amount)
{
    fadeamount = amount;
    oldfadeamount = amount;
}

void Graphics::drawmenu( int cr, int cg, int cb, bool levelmenu /*= false*/ )
{
    for (size_t i = 0; i < game.menuoptions.size(); i++)
    {
        MenuOption& opt = game.menuoptions[i];

        int fr, fg, fb;
        if (opt.active)
        {
            // Color it normally
            fr = cr;
            fg = cg;
            fb = cb;
        }
        else
        {
            // Color it gray
            fr = 128;
            fg = 128;
            fb = 128;
        }

        int x = i*game.menuspacing + game.menuxoff;
        int y = 140 + i*12 + game.menuyoff;

#ifndef NO_CUSTOM_LEVELS
        if (levelmenu)
        {
            size_t separator;
            if (cl.ListOfMetaData.size() > 8)
            {
                separator = 3;
            }
            else
            {
                separator = 1;
            }
            if (game.menuoptions.size() - i <= separator)
            {
                // We're on "next page", "previous page", or "return to menu". Draw them separated by a bit
                y += 8;
            }
            else
            {
                // Get out of the way of the level descriptions
                y += 4;
            }
        }
#endif

        char tempstring[MENU_TEXT_BYTES];
        SDL_strlcpy(tempstring, opt.text, sizeof(tempstring));

        char buffer[MENU_TEXT_BYTES];
        if ((int) i == game.currentmenuoption && game.slidermode == SLIDER_NONE)
        {
            if (opt.active)
            {
                // Uppercase the text
                // FIXME: This isn't UTF-8 aware!
                size_t templen = SDL_strlen(tempstring);
                for (size_t ii = 0; ii < templen; ii++)
                {
                    tempstring[ii] = SDL_toupper(tempstring[ii]);
                }
            }

            // Add brackets
            SDL_snprintf(buffer, sizeof(buffer), "[ %s ]", tempstring);
            // Account for brackets
            x -= 16;
        }
        else
        {
            SDL_strlcpy(buffer, tempstring, sizeof(buffer));
        }

        Print(x, y, buffer, fr, fg, fb);
    }
}


void Graphics::drawcoloredtile(
    const int x,
    const int y,
    const int t,
    const int r,
    const int g,
    const int b
) {
    SDL_Rect rect;

    if (!INBOUNDS_VEC(t, tiles))
    {
        return;
    }

    setcolreal(getRGB(r, g, b));
    setRect(rect, x, y, tiles_rect.w, tiles_rect.h);
    BlitSurfaceColoured(tiles[t], NULL, backBuffer, &rect, ct);
}


bool Graphics::Hitest(SDL_Surface* surface1, point p1, SDL_Surface* surface2, point p2)
{

    //find rectangle where they intersect:

    int r1_left = p1.x;
    int r1_right = r1_left + surface1->w;
    int r2_left = p2.x;
    int r2_right = r2_left + surface2->w;

    int r1_bottom = p1.y;
    int r1_top = p1.y + surface1->h;
    int r2_bottom  = p2.y;
    int r2_top = p2.y + surface2->h;

    SDL_Rect rect1 = {p1.x, p1.y, surface1->w, surface1->h};
    SDL_Rect rect2 = {p2.x, p2.y, surface2->w, surface2->h};
    bool intersection = help.intersects(rect1, rect2);

    if(intersection)
    {
        int r3_left = VVV_max(r1_left, r2_left);
        int r3_top = VVV_min(r1_top, r2_top);
        int r3_right = VVV_min(r1_right, r2_right);
        int r3_bottom= VVV_max(r1_bottom, r2_bottom);

        //for every pixel inside rectangle
        for(int x = r3_left; x < r3_right; x++)
        {
            for(int y = r3_bottom; y < r3_top; y++)
            {
                Uint32 pixel1 = ReadPixel(surface1 , x - p1.x, y - p1.y);
                Uint32 pixel2 = ReadPixel(surface2 , x - p2.x, y - p2.y);
                if ((pixel1 & 0x000000FF) && (pixel2 & 0x000000FF))
                {
                    return true;
                }
            }
        }
    }
    return false;

}

void Graphics::drawgravityline( int t )
{
    if (!INBOUNDS_VEC(t, obj.entities))
    {
        WHINE_ONCE("drawgravityline() out-of-bounds!");
        return;
    }

    if (obj.entities[t].life == 0)
    {
        switch(linestate)
        {
        case 0:
            FillRect(backBuffer,line_rect, getRGB(200-20, 200-20, 200-20));
            break;
        case 1:
            FillRect(backBuffer,line_rect, getRGB(245-30, 245-30, 225-30));
            break;
        case 2:
            FillRect(backBuffer,line_rect, getRGB(225-30, 245-30, 245-30));
            break;
        case 3:
            FillRect(backBuffer,line_rect, getRGB(200-20, 200-20, 164-10));
            break;
        case 4:
            FillRect(backBuffer,line_rect, getRGB(196-20, 255-30, 224-20));
            break;
        case 5:
            FillRect(backBuffer,line_rect, getRGB(196-20, 235-30, 205-20));
            break;
        case 6:
            FillRect(backBuffer,line_rect, getRGB(164-10, 164-10, 164-10));
            break;
        case 7:
            FillRect(backBuffer,line_rect, getRGB(205-20, 245-30, 225-30));
            break;
        case 8:
            FillRect(backBuffer,line_rect, getRGB(225-30, 255-30, 205-20));
            break;
        case 9:
            FillRect(backBuffer,line_rect, getRGB(245-30, 245-30, 245-30));
            break;
        }
    }
    else
    {
        FillRect(backBuffer,line_rect, getRGB(96, 96, 96));
    }
}

void Graphics::drawtrophytext(void)
{
    int temp, temp2, temp3;

    if (obj.trophytext < 15)
    {
        int usethismult = lerp(obj.oldtrophytext, obj.trophytext);
        temp = (196 * usethismult) / 15;
        temp2 = (196 * usethismult) / 15;
        temp3 = ((255 - help.glow) * usethismult) / 15;
    }
    else
    {
        temp = 196;
        temp2 = 196;
        temp3 = 255 - help.glow;
    }
    switch(obj.trophytype)
    {
    case 1:
        Print( -1, 6, "SPACE STATION 1 MASTERED", temp, temp2, temp3, true);
        Print( -1, 16, "Obtain a V Rank in this Time Trial", temp, temp2, temp3, true);
        break;
    case 2:
        Print( -1, 6, "LABORATORY MASTERED", temp, temp2, temp3, true);
        Print( -1, 16, "Obtain a V Rank in this Time Trial", temp, temp2, temp3, true);
        break;
    case 3:
        Print( -1, 6, "THE TOWER MASTERED", temp, temp2, temp3, true);
        Print( -1, 16, "Obtain a V Rank in this Time Trial", temp, temp2, temp3, true);
        break;
    case 4:
        Print( -1, 6, "SPACE STATION 2 MASTERED", temp, temp2, temp3, true);
        Print( -1, 16, "Obtain a V Rank in this Time Trial", temp, temp2, temp3, true);
        break;
    case 5:
        Print( -1, 6, "WARP ZONE MASTERED", temp, temp2, temp3, true);
        Print( -1, 16, "Obtain a V Rank in this Time Trial", temp, temp2, temp3, true);
        break;
    case 6:
        Print( -1, 6, "FINAL LEVEL MASTERED", temp, temp2, temp3, true);
        Print( -1, 16, "Obtain a V Rank in this Time Trial", temp, temp2, temp3, true);
        break;
    case 7:
        Print( -1, 6, "GAME COMPLETE", temp, temp2, temp3, true);
        Print( -1, 16, "Complete the game", temp, temp2, temp3, true);
        break;
    case 8:
        Print( -1, 6, "FLIP MODE COMPLETE", temp, temp2, temp3, true);
        Print( -1, 16, "Complete the game in flip mode", temp, temp2, temp3, true);
        break;
    case 9:
        Print( -1, 11, "Win with less than 50 deaths", temp, temp2, temp3, true);
        break;
    case 10:
        Print( -1, 11, "Win with less than 100 deaths", temp, temp2, temp3, true);
        break;
    case 11:
        Print( -1, 11, "Win with less than 250 deaths", temp, temp2, temp3, true);
        break;
    case 12:
        Print( -1, 11, "Win with less than 500 deaths", temp, temp2, temp3, true);
        break;
    case 13:
        Print( -1, 11, "Last 5 seconds on the Super Gravitron", temp, temp2, temp3, true);
        break;
    case 14:
        Print( -1, 11, "Last 10 seconds on the Super Gravitron", temp, temp2, temp3, true);
        break;
    case 15:
        Print( -1, 11, "Last 15 seconds on the Super Gravitron", temp, temp2, temp3, true);
        break;
    case 16:
        Print( -1, 11, "Last 20 seconds on the Super Gravitron", temp, temp2, temp3, true);
        break;
    case 17:
        Print( -1, 11, "Last 30 seconds on the Super Gravitron", temp, temp2, temp3, true);
        break;
    case 18:
        Print( -1, 11, "Last 1 minute on the Super Gravitron", temp, temp2, temp3, true);
        break;
    case 20:
        Print( -1, 6, "MASTER OF THE UNIVERSE", temp, temp2, temp3, true);
        Print( -1, 16, "Complete the game in no death mode", temp, temp2, temp3, true);
        break;
    }
}

void Graphics::drawentities(void)
{
    const int yoff = map.towermode ? lerp(map.oldypos, map.ypos) : 0;

    if (!map.custommode)
    {
        for (int i = obj.entities.size() - 1; i >= 0; i--)
        {
            if (!obj.entities[i].ishumanoid())
            {
                drawentity(i, yoff);
            }
        }

        for (int i = obj.entities.size() - 1; i >= 0; i--)
        {
            if (obj.entities[i].ishumanoid())
            {
                drawentity(i, yoff);
            }
        }
    }
    else
    {
        for (int i = obj.entities.size() - 1; i >= 0; i--)
        {
            drawentity(i, yoff);
        }
    }
}

void Graphics::drawentity(const int i, const int yoff)
{
    if (!INBOUNDS_VEC(i, obj.entities))
    {
        WHINE_ONCE("drawentity() out-of-bounds!");
        return;
    }

    if (obj.entities[i].invis)
    {
        return;
    }

    point tpoint;

    SDL_Rect drawRect;

#if !defined(NO_CUSTOM_LEVELS)
    // Special case for gray Warp Zone tileset!
    const RoomProperty* const room = cl.getroomprop(game.roomx - 100, game.roomy - 100);
    const bool custom_gray = room->tileset == 3 && room->tilecol == 6;
#else
    const bool custom_gray = false;
#endif

    std::vector<SDL_Surface*>& tilesvec = (map.custommode && !map.finalmode) ? entcolours : tiles;

    std::vector<SDL_Surface*>& spritesvec = flipmode ? flipsprites : sprites;

    const int xp = lerp(obj.entities[i].lerpoldxp, obj.entities[i].xp);
    const int yp = lerp(obj.entities[i].lerpoldyp, obj.entities[i].yp);

    switch (obj.entities[i].size)
    {
    case 0:
    {
        // Sprites
        if (!INBOUNDS_VEC(obj.entities[i].drawframe, spritesvec))
        {
            return;
        }
        tpoint.x = xp;
        tpoint.y = yp - yoff;
        setcolreal(obj.entities[i].realcol);

        drawRect = sprites_rect;
        drawRect.x += tpoint.x;
        drawRect.y += tpoint.y;
        BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe], NULL, backBuffer, &drawRect, ct);

        //screenwrapping!
        point wrappedPoint;
        bool wrapX = false;
        bool wrapY = false;

        wrappedPoint.x = tpoint.x;
        if (tpoint.x < 0)
        {
            wrapX = true;
            wrappedPoint.x += 320;
        }
        else if (tpoint.x > 300)
        {
            wrapX = true;
            wrappedPoint.x -= 320;
        }

        wrappedPoint.y = tpoint.y;
        if (tpoint.y < 8)
        {
            wrapY = true;
            wrappedPoint.y += 232;
        }
        else if (tpoint.y > 210)
        {
            wrapY = true;
            wrappedPoint.y -= 232;
        }

        const bool isInWrappingAreaOfTower = map.towermode && !map.minitowermode && map.ypos >= 500 && map.ypos <= 5000;
        if (wrapX && (map.warpx || isInWrappingAreaOfTower))
        {
            drawRect = sprites_rect;
            drawRect.x += wrappedPoint.x;
            drawRect.y += tpoint.y;
            BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe], NULL, backBuffer, &drawRect, ct);
        }
        if (wrapY && map.warpy)
        {
            drawRect = sprites_rect;
            drawRect.x += tpoint.x;
            drawRect.y += wrappedPoint.y;
            BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe], NULL, backBuffer, &drawRect, ct);
        }
        if (wrapX && wrapY && map.warpx && map.warpy)
        {
            drawRect = sprites_rect;
            drawRect.x += wrappedPoint.x;
            drawRect.y += wrappedPoint.y;
            BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe], NULL, backBuffer, &drawRect, ct);
        }
        break;
    }
    case 1:
        // Tiles
        if (!INBOUNDS_VEC(obj.entities[i].drawframe, tiles))
        {
            return;
        }
        tpoint.x = xp;
        tpoint.y = yp - yoff;
        drawRect = tiles_rect;
        drawRect.x += tpoint.x;
        drawRect.y += tpoint.y;
        BlitSurfaceStandard(tiles[obj.entities[i].drawframe],NULL, backBuffer, &drawRect);
        break;
    case 2:
    case 8:
    {
        // Special: Moving platform, 4 tiles or 8 tiles
        if (!INBOUNDS_VEC(obj.entities[i].drawframe, tilesvec))
        {
            return;
        }
        tpoint.x = xp;
        tpoint.y = yp - yoff;
        int thiswidth = 4;
        if (obj.entities[i].size == 8)
        {
            thiswidth = 8;
        }
        for (int ii = 0; ii < thiswidth; ii++)
        {
            drawRect = tiles_rect;
            drawRect.x += tpoint.x;
            drawRect.y += tpoint.y;
            drawRect.x += 8 * ii;
            if (custom_gray)
            {
                colourTransform temp_ct;
                temp_ct.colour = 0xFFFFFFFF;
                BlitSurfaceTinted(tilesvec[obj.entities[i].drawframe],NULL, backBuffer, &drawRect, temp_ct);
            }
            else
            {
                BlitSurfaceStandard(tilesvec[obj.entities[i].drawframe],NULL, backBuffer, &drawRect);
            }
        }
        break;
    }
    case 3:    // Big chunky pixels!
        prect.x = xp;
        prect.y = yp - yoff;
        FillRect(backBuffer, prect, obj.entities[i].realcol);
        break;
    case 4:    // Small pickups
        setcolreal(obj.entities[i].realcol);
        drawhuetile(xp, yp - yoff, obj.entities[i].tile);
        break;
    case 5:    //Horizontal Line
    {
        int oldw = obj.entities[i].w;
        if ((game.swngame == 3 || kludgeswnlinewidth) && obj.getlineat(84 - 32) == i)
        {
            oldw -= 24;
        }
        line_rect.x = xp;
        line_rect.y = yp - yoff;
        line_rect.w = lerp(oldw, obj.entities[i].w);
        line_rect.h = 1;
        drawgravityline(i);
        break;
    }
    case 6:    //Vertical Line
        line_rect.x = xp;
        line_rect.y = yp - yoff;
        line_rect.w = 1;
        line_rect.h = obj.entities[i].h;
        drawgravityline(i);
        break;
    case 7:    //Teleporter
        drawtele(xp, yp - yoff, obj.entities[i].drawframe, obj.entities[i].realcol);
        break;
    //case 8:    // Special: Moving platform, 8 tiles
        // Note: This code is in the 4-tile code
        break;
    case 9:         // Really Big Sprite! (2x2)
        setcolreal(obj.entities[i].realcol);

        tpoint.x = xp;
        tpoint.y = yp - yoff;

        drawRect = sprites_rect;
        drawRect.x += tpoint.x;
        drawRect.y += tpoint.y;
        if (INBOUNDS_VEC(obj.entities[i].drawframe, spritesvec))
        {
            BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe],NULL, backBuffer, &drawRect, ct);
        }

        tpoint.x = xp+32;
        tpoint.y = yp - yoff;
        //
        drawRect = sprites_rect;
        drawRect.x += tpoint.x;
        drawRect.y += tpoint.y;
        if (INBOUNDS_VEC(obj.entities[i].drawframe+1, spritesvec))
        {
            BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe+1],NULL, backBuffer, &drawRect, ct);
        }

        tpoint.x = xp;
        tpoint.y = yp+32 - yoff;
        //
        drawRect = sprites_rect;
        drawRect.x += tpoint.x;
        drawRect.y += tpoint.y;
        if (INBOUNDS_VEC(obj.entities[i].drawframe+12, spritesvec))
        {
            BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe+12],NULL, backBuffer, &drawRect, ct);
        }

        tpoint.x = xp+32;
        tpoint.y = yp+32 - yoff;
        //
        drawRect = sprites_rect;
        drawRect.x += tpoint.x;
        drawRect.y += tpoint.y;
        if (INBOUNDS_VEC(obj.entities[i].drawframe+13, spritesvec))
        {
            BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe + 13],NULL, backBuffer, &drawRect, ct);
        }
        break;
    case 10:         // 2x1 Sprite
        setcolreal(obj.entities[i].realcol);

        tpoint.x = xp;
        tpoint.y = yp - yoff;
        //
        drawRect = sprites_rect;
        drawRect.x += tpoint.x;
        drawRect.y += tpoint.y;
        if (INBOUNDS_VEC(obj.entities[i].drawframe, spritesvec))
        {
            BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe],NULL, backBuffer, &drawRect, ct);
        }

        tpoint.x = xp+32;
        tpoint.y = yp - yoff;
        //
        drawRect = sprites_rect;
        drawRect.x += tpoint.x;
        drawRect.y += tpoint.y;
        if (INBOUNDS_VEC(obj.entities[i].drawframe+1, spritesvec))
        {
            BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe+1],NULL, backBuffer, &drawRect, ct);
        }
        break;
    case 11:    //The fucking elephant
        setcolreal(obj.entities[i].realcol);
        drawimagecol(3, xp, yp - yoff);
        break;
    case 12:         // Regular sprites that don't wrap
        tpoint.x = xp;
        tpoint.y = yp - yoff;
        setcolreal(obj.entities[i].realcol);
        //
        drawRect = sprites_rect;
        drawRect.x += tpoint.x;
        drawRect.y += tpoint.y;
        if (INBOUNDS_VEC(obj.entities[i].drawframe, spritesvec))
        {
            BlitSurfaceColoured(spritesvec[obj.entities[i].drawframe],NULL, backBuffer, &drawRect, ct);
        }


        //if we're outside the screen, we need to draw indicators

        if (obj.entities[i].xp < -20 && obj.entities[i].vx > 0)
        {
            if (obj.entities[i].xp < -100)
            {
                tpoint.x = -5 + (int(( -xp) / 10));
            }
            else
            {
                tpoint.x = 5;
            }

            tpoint.y = tpoint.y+4;


            drawRect = tiles_rect;
            drawRect.x += tpoint.x;
            drawRect.y += tpoint.y;
            if (INBOUNDS_VEC(1167, tiles))
            {
                BlitSurfaceColoured(tiles[1167],NULL, backBuffer, &drawRect, ct);
            }

        }
        else if (obj.entities[i].xp > 340 && obj.entities[i].vx < 0)
        {
            if (obj.entities[i].xp > 420)
            {
                tpoint.x = 320 - (int(( xp-320) / 10));
            }
            else
            {
                tpoint.x = 310;
            }

            tpoint.y = tpoint.y+4;
            //

            drawRect = tiles_rect;
            drawRect.x += tpoint.x;
            drawRect.y += tpoint.y;
            if (INBOUNDS_VEC(1166, tiles))
            {
                BlitSurfaceColoured(tiles[1166],NULL, backBuffer, &drawRect, ct);
            }
        }
        break;
    case 13:
    {
        //Special for epilogue: huge hero!
        if (!INBOUNDS_VEC(obj.entities[i].drawframe, spritesvec))
        {
            return;
        }

        tpoint.x = xp; tpoint.y = yp - yoff;
        setcolreal(obj.entities[i].realcol);
        setRect(drawRect, xp, yp - yoff, sprites_rect.x * 6, sprites_rect.y * 6);
        SDL_Surface* TempSurface = ScaleSurface( spritesvec[obj.entities[i].drawframe], 6 * sprites_rect.w,6* sprites_rect.h );
        BlitSurfaceColoured(TempSurface, NULL , backBuffer,  &drawRect, ct );
        SDL_FreeSurface(TempSurface);



        break;
    }
    }
}

void Graphics::drawbackground( int t )
{
    int temp = 0;

    switch(t)
    {
    case 1:
        //Starfield
        ClearSurface(backBuffer);
        for (int i = 0; i < numstars; i++)
        {
            stars[i].w = 2;
            stars[i].h = 2;
            SDL_Rect star_rect = stars[i];
            star_rect.x = lerp(star_rect.x + starsspeed[i], star_rect.x);
            if (starsspeed[i] <= 6)
            {
                FillRect(backBuffer,star_rect, getRGB(0x22,0x22,0x22));
            }
            else
            {
                FillRect(backBuffer,star_rect, getRGB(0x55,0x55,0x55));
            }
        }
        break;
    case 2:
    {
        int bcol = 0, bcol2 = 0;

            //Lab
            switch(rcol)
            {
                //Akward ordering to match tileset
            case 0:
                bcol2 = getRGB(0, 16*backboxint[0], 16*backboxint[0]);
                break; //Cyan
            case 1:
                bcol2 = getRGB(16*backboxint[0], 0, 0);
                break;  //Red
            case 2:
                bcol2 = getRGB(16*backboxint[0], 0, 16*backboxint[0]);
                break; //Purple
            case 3:
                bcol2 = getRGB(0, 0, 16*backboxint[0]);
                break;  //Blue
            case 4:
                bcol2 = getRGB(16*backboxint[0], 16*backboxint[0], 0);
                break; //Yellow
            case 5:
                bcol2 = getRGB(0, 16 * backboxint[0], 0);
                break;  //Green
            case 6:
                //crazy case
                switch(spcol)
                {
                case 0:
                    bcol2 = getRGB(0, 16*backboxint[0], 16*backboxint[0]);
                    break; //Cyan
                case 1:
                    bcol2 = getRGB(0, (spcoldel+1)*backboxint[0], 16*backboxint[0]);
                    break; //Cyan
                case 2:
                    bcol2 = getRGB(0, 0, 16*backboxint[0]);
                    break;  //Blue
                case 3:
                    bcol2 = getRGB((16-spcoldel)*backboxint[0], 0, 16*backboxint[0]);
                    break;  //Blue
                case 4:
                    bcol2 = getRGB(16*backboxint[0], 0, 16*backboxint[0]);
                    break; //Purple
                case 5:
                    bcol2 = getRGB(16*backboxint[0], 0, (spcoldel+1)*backboxint[0]);
                    break; //Purple
                case 6:
                    bcol2 = getRGB(16*backboxint[0], 0, 0);
                    break;  //Red
                case 7:
                    bcol2 = getRGB(16*backboxint[0], (16-spcoldel)*backboxint[0], 0);
                    break;  //Red
                case 8:
                    bcol2 = getRGB(16*backboxint[0], 16*backboxint[0], 0);
                    break; //Yellow
                case 9:
                    bcol2 = getRGB((spcoldel+1)*backboxint[0], 16*backboxint[0], 0);
                    break; //Yellow
                case 10:
                    bcol2 = getRGB(0, 16 * backboxint[0], 0);
                    break;  //Green
                case 11:
                    bcol2 = getRGB(0, 16 * backboxint[0], (16-spcoldel)*backboxint[0]);
                    break;  //Green
                }
            break;
        }
        FillRect(backBuffer,bcol2);

        for (int i = 0; i < numbackboxes; i++)
        {
            switch(rcol)
            {
                //Akward ordering to match tileset
            case 0:
                bcol = getRGB(16, 128*backboxint[0], 128*backboxint[0]);
                break; //Cyan
            case 1:
                bcol = getRGB(128*backboxint[0], 16, 16);
                break;  //Red
            case 2:
                bcol = getRGB(128*backboxint[0], 16, 128*backboxint[0]);
                break; //Purple
            case 3:
                bcol = getRGB(16, 16, 128*backboxint[0]);
                break;  //Blue
            case 4:
                bcol = getRGB(128*backboxint[0], 128*backboxint[0], 16);
                break; //Yellow
            case 5:
                bcol = getRGB(16, 128 * backboxint[0], 16);
                break;  //Green
            case 6:
                //crazy case
                switch(spcol)
                {
                case 0:
                    bcol = getRGB(16, 128*backboxint[0], 128*backboxint[0]);
                    break; //Cyan
                case 1:
                    bcol = getRGB(16, ((spcoldel+1)*8)*backboxint[0], 128*backboxint[0]);
                    break; //Cyan
                case 2:
                    bcol = getRGB(16, 16, 128*backboxint[0]);
                    break;  //Blue
                case 3:
                    bcol = getRGB((128-(spcoldel*8))*backboxint[0], 16, 128*backboxint[0]);
                    break;  //Blue
                case 4:
                    bcol = getRGB(128*backboxint[0], 16, 128*backboxint[0]);
                    break; //Purple
                case 5:
                    bcol = getRGB(128*backboxint[0], 16, ((spcoldel+1)*8)*backboxint[0]);
                    break; //Purple
                case 6:
                    bcol = getRGB(128*backboxint[0], 16, 16);
                    break;  //Red
                case 7:
                    bcol = getRGB(128*backboxint[0], (128-(spcoldel*8))*backboxint[0], 16);
                    break;  //Red
                case 8:
                    bcol = getRGB(128*backboxint[0], 128*backboxint[0], 16);
                    break; //Yellow
                case 9:
                    bcol = getRGB(((spcoldel+1)*8)*backboxint[0], 128*backboxint[0], 16);
                    break; //Yellow
                case 10:
                    bcol = getRGB(16, 128 * backboxint[0], 16);
                    break;  //Green
                case 11:
                    bcol = getRGB(16, 128 * backboxint[0], (128-(spcoldel*8))*backboxint[0]);
                    break;  //Green
                }
                break;
            }

            SDL_Rect backboxrect = backboxes[i];
            backboxrect.x = lerp(backboxes[i].x - backboxvx[i], backboxes[i].x);
            backboxrect.y = lerp(backboxes[i].y - backboxvy[i], backboxes[i].y);

            FillRect(backBuffer, backboxrect, bcol);
            backboxrect.x += 1;
            backboxrect.y += 1;
            backboxrect.w -= 2;
            backboxrect.h -= 2;
            FillRect(backBuffer,backboxrect, bcol2);
        }
        break;
    }
    case 3: //Warp zone (horizontal)
        ClearSurface(backBuffer);
        BlitSurfaceStandard(warpbuffer, NULL, warpbuffer_lerp, NULL);
        ScrollSurface(warpbuffer_lerp, lerp(0, -3), 0);
        BlitSurfaceStandard(warpbuffer_lerp, &towerbuffer_rect, backBuffer, NULL);
        break;
    case 4: //Warp zone (vertical)
        ClearSurface(backBuffer);
        SDL_BlitSurface(warpbuffer, NULL, warpbuffer_lerp, NULL);
        ScrollSurface(warpbuffer_lerp, 0, lerp(0, -3));
        SDL_BlitSurface(warpbuffer_lerp, &towerbuffer_rect, backBuffer, NULL);
        break;
    case 5:
        //Warp zone, central
        switch(rcol)
        {
            //Akward ordering to match tileset
        case 0:
            warpbcol = getRGB(0x0A, 0x10, 0x0E);
            warpfcol = getRGB(0x10, 0x22, 0x21);
            break; //Cyan
        case 1:
            warpbcol = getRGB(0x11, 0x09, 0x0B);
            warpfcol = getRGB(0x22, 0x10, 0x11);
            break; //Red
        case 2:
            warpbcol = getRGB(0x0F, 0x0A, 0x10);
            warpfcol = getRGB(0x22,0x10,0x22);
            break; //Purple
        case 3:
            warpbcol = getRGB(0x0A, 0x0B, 0x10);
            warpfcol = getRGB(0x10, 0x10, 0x22);
            break; //Blue
        case 4:
            warpbcol = getRGB(0x10, 0x0D, 0x0A);
            warpfcol = getRGB(0x22, 0x1E, 0x10);
            break; //Yellow
        case 5:
            warpbcol = getRGB(0x0D, 0x10, 0x0A);
            warpfcol = getRGB(0x14, 0x22, 0x10);
            break; //Green
        case 6:
            warpbcol = getRGB(0x0A, 0x0A, 0x0A);
            warpfcol = getRGB(0x12, 0x12, 0x12);
            break; //Gray
        default:
            warpbcol = getRGB(0xFF, 0xFF, 0xFF);
            warpfcol = getRGB(0xFF, 0xFF, 0xFF);
        }

        for (int i = 10 ; i >= 0; i--)
        {
            temp = (i << 4) + backoffset;
            setwarprect(160 - temp, 120 - temp, temp * 2, temp * 2);
            if (i % 2 == warpskip)
            {
                FillRect(backBuffer, warprect, warpbcol);
            }
            else
            {
                FillRect(backBuffer,warprect, warpfcol);
            }
        }
        break;
    case 6:
        //Final Starfield
        ClearSurface(backBuffer);
        for (int i = 0; i < numstars; i++)
        {
            stars[i].w = 2;
            stars[i].h = 2;
            SDL_Rect star_rect = stars[i];
            star_rect.y = lerp(star_rect.y + starsspeed[i], star_rect.y);
            if (starsspeed[i] <= 8)
            {
                FillRect(backBuffer, star_rect, getRGB(0x22, 0x22, 0x22));
            }
            else
            {
                FillRect(backBuffer, star_rect, getRGB(0x55, 0x55, 0x55));
            }
        }
        break;
    case 7:
        //Static, unscrolling section of the tower
        for (int j = 0; j < 30; j++)
        {
            for (int i = 0; i < 40; i++)
            {
                drawtile3(i * 8, j * 8, map.tower.backat(i, j, 200), 15);
            }
        }
        break;
    case 8:
        //Static, unscrolling section of the tower
        for (int j = 0; j < 30; j++)
        {
            for (int i = 0; i < 40; i++)
            {
                drawtile3(i * 8, j * 8, map.tower.backat(i, j, 200), 10);
            }
        }
        break;
    case 9:
        //Static, unscrolling section of the tower
        for (int j = 0; j < 30; j++)
        {
            for (int i = 0; i < 40; i++)
            {
                drawtile3(i * 8, j * 8, map.tower.backat(i, j, 600), 0);
            }
        }
        break;
    default:
        ClearSurface(backBuffer);

        break;
    }
}

void Graphics::updatebackground(int t)
{
    switch (t)
    {
    case 1:
        //Starfield
        for (int i = 0; i < numstars; i++)
        {
            stars[i].w = 2;
            stars[i].h = 2;
            stars[i].x -= starsspeed[i];
            if (stars[i].x < -10)
            {
                stars[i].x += 340;
                stars[i].y = int(fRandom() * 240);
                stars[i].w = 2;
                starsspeed[i] = 4+int(fRandom()*4);
            }
        }
        break;
    case 2:
        //Lab
        if (rcol == 6)
        {
            //crazy caze
            spcoldel--;
            if (spcoldel <= 0)
            {
                spcoldel = 15;
                spcol++;
                if (spcol >= 12) spcol = 0;
            }
        }
        for (int i = 0; i < numbackboxes; i++)
        {
            backboxes[i].x += backboxvx[i];
            backboxes[i].y += backboxvy[i];
            if (backboxes[i].x < -40)
            {
                backboxes[i].x = 320;
                backboxes[i].y = fRandom() * 240;
            }
            if (backboxes[i].x > 320)
            {
                backboxes[i].x = -32;
                backboxes[i].y = fRandom() * 240;
            }
            if (backboxes[i].y < -40)
            {
                backboxes[i].y = 240;
                backboxes[i].x = fRandom() * 320;
            }
            if (backboxes[i].y > 260)
            {
                backboxes[i].y = -32;
                backboxes[i].x = fRandom() * 320;
            }
        }
        break;
    case 3: //Warp zone (horizontal)
    {
        int temp = 680 + (rcol * 3);
        backoffset+=3;
        if (backoffset >= 16) backoffset -= 16;

        if (backgrounddrawn)
        {
            ScrollSurface(warpbuffer, -3, 0 );
            for (int j = 0; j < 15; j++)
            {
                for (int i = 0; i < 2; i++)
                {
                    drawtowertile(317 - backoffset + (i * 16), (j * 16), temp+40);  //20*16 = 320
                    drawtowertile(317 - backoffset + (i * 16) + 8, (j * 16), temp + 41);
                    drawtowertile(317 - backoffset + (i * 16), (j * 16) + 8, temp + 80);
                    drawtowertile(317 - backoffset + (i * 16) + 8, (j * 16) + 8, temp + 81);
                }
            }
        }
        else
        {
            //draw the whole thing for the first time!
            backoffset = 0;
            ClearSurface(warpbuffer);
            for (int j = 0; j < 15; j++)
            {
                for (int i = 0; i < 21; i++)
                {
                    drawtowertile((i * 16) - backoffset - 3, (j * 16), temp+40);
                    drawtowertile((i * 16) - backoffset + 8 - 3, (j * 16), temp + 41);
                    drawtowertile((i * 16) - backoffset - 3, (j * 16) + 8, temp + 80);
                    drawtowertile((i * 16) - backoffset + 8 - 3, (j * 16) + 8, temp + 81);
                }
            }
            backgrounddrawn = true;
        }
        break;
    }
    case 4: //Warp zone (vertical)
    {
        int temp = 760 + (rcol * 3);
        backoffset+=3;
        if (backoffset >= 16) backoffset -= 16;

        if (backgrounddrawn)
        {
            ScrollSurface(warpbuffer,0,-3);
            for (int j = 0; j < 2; j++)
            {
                for (int i = 0; i < 21; i++)
                {
                    drawtowertile((i * 16), 237 - backoffset + (j * 16), temp + 40); //14*17=240 - 3
                    drawtowertile((i * 16) + 8, 237 - backoffset + (j * 16), temp + 41);
                    drawtowertile((i * 16), 237 - backoffset + (j * 16) + 8, temp + 80);
                    drawtowertile((i * 16) + 8, 237 - backoffset + (j * 16) + 8, temp + 81);
                }
            }
        }
        else
        {
            //draw the whole thing for the first time!
            backoffset = 0;
            ClearSurface(warpbuffer);
            for (int j = 0; j < 16; j++)
            {
                for (int i = 0; i < 21; i++)
                {
                    drawtowertile((i * 16), (j * 16)- backoffset - 3, temp+40);
                    drawtowertile((i * 16)+ 8, (j * 16)- backoffset - 3, temp + 41);
                    drawtowertile((i * 16), (j * 16)- backoffset + 8 - 3, temp + 80);
                    drawtowertile((i * 16)+ 8, (j * 16)- backoffset + 8 - 3, temp + 81);
                }
            }
            backgrounddrawn = true;
        }
        break;
    }
    case 5:
        //Warp zone, central

        backoffset += 1;
        if (backoffset >= 16)
        {
            backoffset -= 16;
            warpskip = (warpskip + 1) % 2;
        }
        break;
    case 6:
        //Final Starfield
        for (int i = 0; i < numstars; i++)
        {
            stars[i].w = 2;
            stars[i].h = 2;
            stars[i].y -= starsspeed[i];
            if (stars[i].y < -10)
            {
                stars[i].y += 260;
                stars[i].x = fRandom() * 320;
                starsspeed[i] = 5+(fRandom()*5);
            }
        }
        break;
    }
}

void Graphics::drawmap(void)
{
    if (!foregrounddrawn)
    {
        ClearSurface(foregroundBuffer);
        if(map.tileset==0)
        {
            for (int j = 0; j < 30; j++)
            {
                for (int i = 0; i < 40; i++)
                {
                    const int tile = map.contents[TILE_IDX(i, j)];
                    if(tile>0) drawforetile(i * 8, j * 8, tile);
                }
            }
        }
        else if (map.tileset == 1)
        {
            for (int jt = 0; jt < 30; jt++)
            {
                for (int it = 0; it < 40; it++)
                {
                    const int tile = map.contents[TILE_IDX(it, jt)];
                    if(tile>0) drawforetile2(it * 8, jt * 8, tile);
                }
            }
        }
        else if (map.tileset == 2)
        {
            for (int j = 0; j < 30; j++)
            {
                for (int i = 0; i < 40; i++)
                {
                    const int tile = map.contents[TILE_IDX(i, j)];
                    if(tile>0) drawforetile3(i * 8, j * 8, tile,map.rcol);
                }
            }
        }
        foregrounddrawn = true;
    }
    SDL_BlitSurface(foregroundBuffer, NULL, backBuffer, NULL);

}

void Graphics::drawfinalmap(void)
{
    if (!foregrounddrawn) {
        ClearSurface(foregroundBuffer);
        if(map.tileset==0){
            for (int j = 0; j < 30; j++) {
                for (int i = 0; i < 40; i++) {
                    if((map.contents[TILE_IDX(i, j)])>0)
                        drawforetile(i * 8, j * 8, map.finalat(i,j));
                }
            }
        }else if (map.tileset == 1) {
            for (int j = 0; j < 30; j++) {
                for (int i = 0; i < 40; i++) {
                    if((map.contents[TILE_IDX(i, j)])>0)
                        drawforetile2(i * 8, j * 8, map.finalat(i,j));
                }
            }
        }
        foregrounddrawn=true;
    }

    SDL_BlitSurface(foregroundBuffer, NULL, backBuffer, NULL);
}

void Graphics::drawtowermap(void)
{
    int temp;
    int yoff = lerp(map.oldypos, map.ypos);
    for (int j = 0; j < 31; j++)
    {
        for (int i = 0; i < 40; i++)
        {
            temp = map.tower.at(i, j, yoff);
            if (temp > 0) drawtile3(i * 8, (j * 8) - (yoff % 8), temp, towerbg.colstate);
        }
    }
}

void Graphics::drawtowerspikes(void)
{
    int spikeleveltop = lerp(map.oldspikeleveltop, map.spikeleveltop);
    int spikelevelbottom = lerp(map.oldspikelevelbottom, map.spikelevelbottom);
    for (int i = 0; i < 40; i++)
    {
        drawtile3(i * 8, -8+spikeleveltop, 9, towerbg.colstate);
        drawtile3(i * 8, 230-spikelevelbottom, 8, towerbg.colstate, 8 - spikelevelbottom);
    }
}

void Graphics::drawtowerbackground(const TowerBG& bg_obj)
{
    ClearSurface(backBuffer);
    SDL_BlitSurface(bg_obj.buffer, NULL, bg_obj.buffer_lerp, NULL);
    ScrollSurface(bg_obj.buffer_lerp, 0, lerp(0, -bg_obj.bscroll));
    SDL_BlitSurface(bg_obj.buffer_lerp, &towerbuffer_rect, backBuffer, NULL);
}

void Graphics::updatetowerbackground(TowerBG& bg_obj)
{
    int temp;

    if (bg_obj.bypos < 0) bg_obj.bypos += 120 * 8;

    if (bg_obj.tdrawback)
    {
        int off = bg_obj.scrolldir == 0 ? 0 : bg_obj.bscroll;
        //Draw the whole thing; needed for every colour cycle!
        for (int j = -1; j < 32; j++)
        {
            for (int i = 0; i < 40; i++)
            {
                temp = map.tower.backat(i, j, bg_obj.bypos);
                drawtowertile3(i * 8, (j * 8) - (bg_obj.bypos % 8) - off, temp, bg_obj);
            }
        }

        bg_obj.tdrawback = false;
    }
    else
    {
        //just update the bottom
        ScrollSurface(bg_obj.buffer, 0, -bg_obj.bscroll);
        if (bg_obj.scrolldir == 0)
        {
            for (int i = 0; i < 40; i++)
            {
                temp = map.tower.backat(i, -1, bg_obj.bypos);
                drawtowertile3(i * 8, -1*8 - (bg_obj.bypos % 8), temp, bg_obj);
                temp = map.tower.backat(i, 0, bg_obj.bypos);
                drawtowertile3(i * 8, -(bg_obj.bypos % 8), temp, bg_obj);
            }
        }
        else
        {
            for (int i = 0; i < 40; i++)
            {
                temp = map.tower.backat(i, 29, bg_obj.bypos);
                drawtowertile3(i * 8, 29*8 - (bg_obj.bypos % 8) - bg_obj.bscroll, temp, bg_obj);
                temp = map.tower.backat(i, 30, bg_obj.bypos);
                drawtowertile3(i * 8, 30*8 - (bg_obj.bypos % 8) - bg_obj.bscroll, temp, bg_obj);
                temp = map.tower.backat(i, 31, bg_obj.bypos);
                drawtowertile3(i * 8, 31*8 - (bg_obj.bypos % 8) - bg_obj.bscroll, temp, bg_obj);
                temp = map.tower.backat(i, 32, bg_obj.bypos);
                drawtowertile3(i * 8, 32*8 - (bg_obj.bypos % 8) - bg_obj.bscroll, temp, bg_obj);
            }
        }
    }
}

void Graphics::setcol( int t )
{
    int temp;

    //Setup predefinied colours as per our zany palette
    switch(t)
    {
        //Player Normal
    case 0:
        ct.colour = getRGB(160- help.glow/2 - (fRandom()*20), 200- help.glow/2, 220 - help.glow);
        break;
        //Player Hurt
    case 1:
        ct.colour = getRGB(196 - (fRandom() * 64), 10, 10);
        break;
        //Enemies and stuff
    case 2:
        ct.colour = getRGB(225-(help.glow/2), 75, 30);
        break;
    case 3: //Trinket
        if (!trinketcolset)
        {
            trinketr = 200 - (fRandom() * 64);
            trinketg = 200 - (fRandom() * 128);
            trinketb = 164 + (fRandom() * 60);
            trinketcolset = true;
        }
        ct.colour = getRGB(trinketr, trinketg, trinketb);
        break;
    case 4: //Inactive savepoint
        temp = (help.glow/2) + (fRandom() * 8);
        ct.colour = getRGB(80 + temp, 80 + temp, 80 + temp);
        break;
    case 5: //Active savepoint
        ct.colour = getRGB(164+(fRandom()*64),164+(fRandom()*64), 255-(fRandom()*64));
        break;
    case 6: //Enemy : Red
        ct.colour = getRGB(250 - help.glow/2, 60- help.glow/2, 60 - help.glow/2);
        break;
    case 7: //Enemy : Green
        ct.colour = getRGB(100 - help.glow/2 - (fRandom()*30), 250 - help.glow/2, 100 - help.glow/2 - (fRandom()*30));
        break;
    case 8: //Enemy : Purple
        ct.colour = getRGB(250 - help.glow/2, 20, 128 - help.glow/2 + (fRandom()*30));
        break;
    case 9: //Enemy : Yellow
        ct.colour = getRGB(250 - help.glow/2, 250 - help.glow/2, 20);
        break;
    case 10: //Warp point (white)
        ct.colour = getRGB(255 - (fRandom() * 64), 255 - (fRandom() * 64), 255 - (fRandom() * 64));
        break;
    case 11: //Enemy : Cyan
        ct.colour = getRGB(20, 250 - help.glow/2, 250 - help.glow/2);
        break;
    case 12: //Enemy : Blue
        ct.colour = getRGB(90- help.glow/2, 90 - help.glow/2, 250 - help.glow/2);
        break;
        //Crew Members
        //green
    case 13:
        ct.colour = getRGB(120- help.glow/4 - (fRandom()*20), 220 - help.glow/4, 120- help.glow/4);
        break;
        //Yellow
    case 14:
        ct.colour = getRGB(220- help.glow/4 - (fRandom()*20), 210 - help.glow/4, 120- help.glow/4);
        break;
        //pink
    case 15:
        ct.colour = getRGB(255 - help.glow/8, 70 - help.glow/4, 70 - help.glow / 4);
        break;
        //Blue
    case 16:
        ct.colour = getRGB(75, 75, 255- help.glow/4 - (fRandom()*20));
        break;


    case 17: //Enemy : Orange
        ct.colour = getRGB(250 - help.glow/2, 130 - help.glow/2, 20);
        break;
    case 18: //Enemy : Gray
        ct.colour = getRGB(130- help.glow/2, 130 - help.glow/2, 130 - help.glow/2);
        break;
    case 19: //Enemy : Dark gray
        ct.colour = getRGB(60- help.glow/8, 60 - help.glow/8, 60 - help.glow/8);
        break;
        //Purple
    case 20:
        ct.colour = getRGB(220 - help.glow / 4 - (fRandom() * 20), 120 - help.glow / 4, 210 - help.glow / 4);
        break;

    case 21: //Enemy : Light Gray
        ct.colour = getRGB(180- help.glow/2, 180 - help.glow/2, 180 - help.glow/2);
        break;
    case 22: //Enemy : Indicator Gray
        ct.colour = getRGB(230- help.glow/2, 230- help.glow/2, 230- help.glow/2);
        break;
    case 23: //Enemy : Indicator Gray
        ct.colour = getRGB(255- help.glow/2 - (fRandom() * 40) , 255- help.glow/2 - (fRandom() * 40), 255- help.glow/2 - (fRandom() * 40));
        break;

        //Trophies
        //Yellow
    case 30:
        ct.colour = RGBf(160, 200, 220);
        break;
        //Purple
    case 31:
        ct.colour = RGBf(220, 120, 210);
        break;
        //cyan
    case 32:
        ct.colour = RGBf(220, 210, 120);
        break;
        //Blue
    case 33:
        ct.colour = RGBf(255, 70, 70);
        break;
        //green
    case 34:
        ct.colour = RGBf(120, 220, 120);
        break;
        //red
    case 35:
        ct.colour = RGBf(75, 75, 255);
        break;
        //Gold
    case 36:
        ct.colour = getRGB(180, 120, 20);
        break;
    case 37: //Trinket
        if (!trinketcolset)
        {
            trinketr = 200 - (fRandom() * 64);
            trinketg = 200 - (fRandom() * 128);
            trinketb = 164 + (fRandom() * 60);
            trinketcolset = true;
        }
        ct.colour = RGBf(trinketr, trinketg, trinketb);
        break;
        //Silver
    case 38:
        ct.colour = RGBf(196, 196, 196);
        break;
        //Bronze
    case 39:
        ct.colour = RGBf(128, 64, 10);
        break;
        //Awesome
    case 40: //Teleporter in action!
        temp = fRandom() * 150;
        if(temp<33)
        {
            ct.colour = RGBf(255 - (fRandom() * 64), 64 + (fRandom() * 64), 64 + (fRandom() * 64));
        }
        else if (temp < 66)
        {
            ct.colour = RGBf(64 + (fRandom() * 64), 255 - (fRandom() * 64), 64 + (fRandom() * 64));
        }
        else if (temp < 100)
        {
            ct.colour = RGBf(64 + (fRandom() * 64), 64 + (fRandom() * 64), 255 - (fRandom() * 64));
        }
        else
        {
            ct.colour = RGBf(164+(fRandom()*64),164+(fRandom()*64), 255-(fRandom()*64));
        }
        break;

    case 100: //Inactive Teleporter
        temp = (help.glow/2) + (fRandom() * 8);
        ct.colour = getRGB(42 + temp, 42 + temp, 42 + temp);
        break;
    case 101: //Active Teleporter
        ct.colour = getRGB(164+(fRandom()*64),164+(fRandom()*64), 255-(fRandom()*64));
        break;
    case 102: //Teleporter in action!
        temp = fRandom() * 150;
        if(temp<33)
        {
            ct.colour = getRGB(255 - (fRandom() * 64), 64 + (fRandom() * 64), 64 + (fRandom() * 64));
        }
        else if (temp < 66)
        {
            ct.colour = getRGB(64 + (fRandom() * 64), 255 - (fRandom() * 64), 64 + (fRandom() * 64));
        }
        else if (temp < 100)
        {
            ct.colour = getRGB(64 + (fRandom() * 64), 64 + (fRandom() * 64), 255 - (fRandom() * 64));
        }
        else
        {
            ct.colour = getRGB(164+(fRandom()*64),164+(fRandom()*64), 255-(fRandom()*64));
        }
        break;

    default:
        ct.colour = getRGB(255, 255, 255);
        break;
    }
}

void Graphics::menuoffrender(void)
{
    const int usethisoffset = lerp(oldmenuoffset, menuoffset);
    SDL_Rect offsetRect = {0, usethisoffset, backBuffer.Width, backBuffer.Height};

    BlitSurfaceStandard(backBuffer, NULL, menubuffer, NULL);
    BlitSurfaceStandard(tempBuffer, NULL, backBuffer, NULL);
    BlitSurfaceStandard(menubuffer, NULL, backBuffer, &offsetRect);

    screenbuffer->UpdateScreen(backBuffer, NULL);
    ClearSurface(backBuffer);
}

void Graphics::drawhuetile( int x, int y, int t )
{
    if (!INBOUNDS_VEC(t, tiles))
    {
        return;
    }
    point tpoint;
    tpoint.x = x;
    tpoint.y = y;


    SDL_Rect rect;
    setRect(rect,tpoint.x,tpoint.y,tiles_rect.w, tiles_rect.h);
    BlitSurfaceColoured(tiles[t],NULL,backBuffer, &rect, ct);
}

void Graphics::huetilesetcol(int t)
{
    switch (t)
    {
    case 0:
        setcolreal(getRGB(250-int(fRandom()*32), 250-int(fRandom()*32), 10));
        break;
    case 1:
        setcolreal(getRGB(250-int(fRandom()*32), 250-int(fRandom()*32), 10));
        break;
    default:
        setcolreal(getRGB(250-int(fRandom()*32), 250-int(fRandom()*32),  10));
        break;
    }
}

Uint32 Graphics::bigchunkygetcol(int t)
{
    //A seperate index of colours, for simplicity
    switch (t)
    {
    case 1:
        return getRGB((fRandom() * 64), 10, 10);
    case 2:
        return getRGB(int(160- help.glow/2 - (fRandom()*20)),  200- help.glow/2, 220 - help.glow);
    }
    return 0x00000000;
}

void Graphics::setwarprect( int a, int b, int c, int d )
{
    warprect.x = a;
    warprect.y = b;
    warprect.w = c;
    warprect.h = d;
}

void Graphics::textboxcenterx(void)
{
    if (!INBOUNDS_VEC(m, textboxes))
    {
        vlog_error("textboxcenterx() out-of-bounds!");
        return;
    }

    textboxes[m].centerx();
}

int Graphics::textboxwidth(void)
{
    if (!INBOUNDS_VEC(m, textboxes))
    {
        vlog_error("textboxwidth() out-of-bounds!");
        return 0;
    }

    return textboxes[m].w;
}

void Graphics::textboxmoveto(int xo)
{
    if (!INBOUNDS_VEC(m, textboxes))
    {
        vlog_error("textboxmoveto() out-of-bounds!");
        return;
    }

    textboxes[m].xp = xo;
}

void Graphics::textboxcentery(void)
{
    if (!INBOUNDS_VEC(m, textboxes))
    {
        vlog_error("textboxcentery() out-of-bounds!");
        return;
    }

    textboxes[m].centery();
}

int Graphics::crewcolour(const int t)
{
    //given crewmate t, return colour in setcol
    if (t == 0) return CYAN;
    if (t == 1) return PURPLE;
    if (t == 2) return YELLOW;
    if (t == 3) return RED;
    if (t == 4) return GREEN;
    if (t == 5) return BLUE;
    return 0;
}

void Graphics::flashlight(void)
{
    FillRect(backBuffer, 0xBBBBBBBB);
}

void Graphics::screenshake(void)
{
    SDL_Rect shakeRect = {screenshake_x, screenshake_y, backBuffer.Width, backBuffer.Height};
    screenbuffer->UpdateScreen(backBuffer, &shakeRect);

    ClearSurface(backBuffer);
}

void Graphics::updatescreenshake(void)
{
    screenshake_x =  static_cast<Sint32>((fRandom() * 7) - 4);
    screenshake_y =  static_cast<Sint32>((fRandom() * 7) - 4);
}

void Graphics::render(void)
{
    if (screenbuffer == NULL)
    {
        return;
    }

    screenbuffer->UpdateScreen(backBuffer, NULL);
}

void Graphics::renderwithscreeneffects(void)
{
    if (game.flashlight > 0 && !game.noflashingmode)
    {
        flashlight();
    }

    if (game.screenshake > 0 && !game.noflashingmode)
    {
        screenshake();
    }
    else
    {
        render();
    }
}

void Graphics::renderfixedpre(void)
{
    if (game.screenshake > 0)
    {
        updatescreenshake();
    }

    if (screenbuffer != NULL && screenbuffer->badSignalEffect)
    {
        UpdateFilter();
    }
}

void Graphics::renderfixedpost(void)
{
    /* Screen effects timers */
    if (game.flashlight > 0)
    {
        --game.flashlight;
    }
    if (game.screenshake > 0)
    {
        --game.screenshake;
    }
}

void Graphics::bigrprint(int x, int y, const std::string& t, int r, int g, int b, bool cen, float sc)
{
    x = x /  (sc);

    x -= (len(t));

    if (cen)
    {
        x = VVV_max(160 - (int((len(t)/ 2.0)*sc)), 0 );
    }
    else
    {
        x *=  (sc);
    }

    return do_print(x, y, t, r, g, b, 255, sc);
}

void Graphics::bigbrprint(int x, int y, const std::string& s, int r, int g, int b, bool cen, float sc)
{
    if (!notextoutline)
    {
        int x_o = x / sc - len(s);
        bigrprint(x, y - sc, s, 0, 0, 0, cen, sc);
        if (cen)
        {
            x_o = VVV_max(160 - (len(s) / 2) * sc, 0);
            bigprint(x_o - sc, y, s, 0, 0, 0, false, sc);
            bigprint(x_o + sc, y, s, 0, 0, 0, false, sc);
        }
        else
        {
            x_o *= sc;
            bigprint(x_o - sc, y, s, 0, 0, 0, false, sc);
            bigprint(x_o + sc, y, s, 0, 0, 0, false, sc);
        }
        bigrprint(x, y + sc, s, 0, 0, 0, cen, sc);
    }

    bigrprint(x, y, s, r, g, b, cen, sc);
}

void Graphics::drawtele(int x, int y, int t, Uint32 c)
{
    setcolreal(getRGB(16,16,16));

    SDL_Rect telerect;
    setRect(telerect, x , y, tele_rect.w, tele_rect.h );
    if (INBOUNDS_VEC(0, tele))
    {
        BlitSurfaceColoured(tele[0], NULL, backBuffer, &telerect, ct);
    }

    setcolreal(c);
    if (t > 9) t = 8;
    if (t < 1) t = 1;

    setRect(telerect, x , y, tele_rect.w, tele_rect.h );
    if (INBOUNDS_VEC(t, tele))
    {
        BlitSurfaceColoured(tele[t], NULL, backBuffer, &telerect, ct);
    }
}

Uint32 Graphics::getRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    return SDL_MapRGBA(backBuffer.surface->format, r, g, b, a);
}

Uint32 Graphics::getRGB(Uint8 r, Uint8 g, Uint8 b)
{
    return SDL_MapRGB(backBuffer.surface->format, r, g, b);
}

Uint32 Graphics::getRGB(Uint32 _col)
{
    return ( _col);
}

Uint32 Graphics::RGBf(int r, int g, int b)
{
    r = (r+128) / 3;
    g = (g+128) / 3;
    b = (b+128) / 3;
    return SDL_MapRGB(backBuffer.surface->format, r, g, b);
}

void Graphics::setcolreal(Uint32 t)
{
    ct.colour = t;
}

void Graphics::drawforetile(int x, int y, int t)
{
    if (!INBOUNDS_VEC(t, tiles))
    {
        WHINE_ONCE("drawforetile() out-of-bounds!");
        return;
    }

    SDL_Rect rect;
    setRect(rect, x,y,tiles_rect.w, tiles_rect.h);

#if !defined(NO_CUSTOM_LEVELS)
    if (shouldrecoloroneway(t, tiles1_mounted))
    {
        colourTransform thect = {cl.getonewaycol()};
        BlitSurfaceTinted(tiles[t], NULL, foregroundBuffer, &rect, thect);
    }
    else
#endif
    {
        BlitSurfaceStandard(tiles[t],NULL, foregroundBuffer, &rect  );
    }
}

void Graphics::drawforetile2(int x, int y, int t)
{
    if (!INBOUNDS_VEC(t, tiles2))
    {
        WHINE_ONCE("drawforetile2() out-of-bounds!");
        return;
    }

    SDL_Rect rect;
    setRect(rect, x,y,tiles_rect.w, tiles_rect.h);

#if !defined(NO_CUSTOM_LEVELS)
    if (shouldrecoloroneway(t, tiles2_mounted))
    {
        colourTransform thect = {cl.getonewaycol()};
        BlitSurfaceTinted(tiles2[t], NULL, foregroundBuffer, &rect, thect);
    }
    else
#endif
    {
        BlitSurfaceStandard(tiles2[t],NULL, foregroundBuffer, &rect  );
    }
}

void Graphics::drawforetile3(int x, int y, int t, int off)
{
    t += off * 30;
    if (!INBOUNDS_VEC(t, tiles3))
    {
        WHINE_ONCE("drawforetile3() out-of-bounds!");
        return;
    }
    SDL_Rect rect;
    setRect(rect, x,y,tiles_rect.w, tiles_rect.h);
    BlitSurfaceStandard(tiles3[t],NULL, foregroundBuffer, &rect  );
}

void Graphics::drawrect(int x, int y, int w, int h, int r, int g, int b)
{
    SDL_Rect madrect;
    //Draw the retangle indicated by that object
    madrect.x = x;
    madrect.y = y;
    madrect.w = w;
    madrect.h = 1;
    FillRect(backBuffer, madrect, getRGB(r, g, b));

    madrect.w = 1;
    madrect.h = h;
    FillRect(backBuffer, madrect, getRGB(r, g, b));

    madrect.x = x + w - 1;
    madrect.w = 1;
    madrect.h = h;
    FillRect(backBuffer, madrect, getRGB(r, g, b));
    madrect.x = x;
    madrect.y = y + h - 1;
    madrect.w = w;
    madrect.h = 1;
    FillRect(backBuffer, madrect, getRGB(r, g, b));
}

bool Graphics::onscreen(int t)
{
    return (t >= -40 && t <= 280);
}

bool Graphics::reloadresources(void)
{
    grphx.destroy();
    grphx.init();

    destroy();

    vlog_info("GFX | Initializing arrays");
    MAYBE_FAIL(MakeTileArray());
    MAYBE_FAIL(MakeSpriteArray());
    MAYBE_FAIL(maketelearray());
    MAYBE_FAIL(Makebfont());

    vlog_info("GFX | Clearing images");
    images.clear();

    vlog_info("GFX | Creating images");
    images.push_back(grphx.im_image0);
    images.push_back(grphx.im_image1);
    images.push_back(grphx.im_image2);
    images.push_back(grphx.im_image3);
    images.push_back(grphx.im_image4);
    images.push_back(grphx.im_image5);
    images.push_back(grphx.im_image6);

    images.push_back(grphx.im_image7);
    images.push_back(grphx.im_image8);
    images.push_back(grphx.im_image9);
    images.push_back(grphx.im_image10);
    images.push_back(grphx.im_image11);
    images.push_back(grphx.im_image12);

    // I wonder why graphics are responsible for loading music.
    vlog_info("GFX | Initializing music");
    music.destroy();
    music.init();

#ifndef NO_CUSTOM_LEVELS
    vlog_info("GFX | Checking assets");
    tiles1_mounted = FILESYSTEM_isAssetMounted("graphics/tiles.png");
    tiles2_mounted = FILESYSTEM_isAssetMounted("graphics/tiles2.png");
    minimap_mounted = FILESYSTEM_isAssetMounted("graphics/minimap.png");
#endif

    return true;

fail:
    return false;
}

Uint32 Graphics::crewcolourreal(int t)
{
    switch (t)
    {
    case 0:
        return col_crewcyan;
    case 1:
        return col_crewpurple;
    case 2:
        return col_crewyellow;
    case 3:
        return col_crewred;
    case 4:
        return col_crewgreen;
    case 5:
        return col_crewblue;
    }
    return col_crewcyan;
}