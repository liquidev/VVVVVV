#ifndef UTILITYCLASS_H
#define UTILITYCLASS_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>

int ss_toi(const std::string& str);

bool next_split(
    size_t* start,
    size_t* len,
    const char* str,
    const char delim
);

bool next_split_s(
    char buffer[],
    const size_t buffer_size,
    size_t* start,
    const char* str,
    const char delim
);

bool is_number(const char* str);

bool is_positive_num(const char* str, const bool hex);

bool endsWith(const char* str, const char* suffix);

void VVV_fillstring(
    char* buffer,
    const size_t buffer_size,
    const char fillchar
);

#define INBOUNDS_VEC(index, vector) ((int) index >= 0 && (int) index < (int) vector.size())
#define INBOUNDS_ARR(index, array) ((int) index >= 0 && (int) index < (int) SDL_arraysize(array))

#define WHINE_ONCE(message) \
    static bool whine = true; \
    if (whine) \
    { \
        whine = false; \
        vlog_error(message); \
    } \
    do { } while (false)

/* Don't call this directly; use the VVV_between macro. */
void _VVV_between(
    const char* original,
    const size_t left_length,
    char* middle,
    const size_t right_length,
    const size_t middle_size
);

/* If original is "LEFTMIDDLERIGHT", VVV_between(original, "LEFT", buffer, "RIGHT")
 * will put "MIDDLE" into buffer - assuming that sizeof(buffer) refers to length
 * of buffer and not length of pointer to buffer.
 */
#define VVV_between(original, left, middle, right) \
    _VVV_between( \
        original, \
        SDL_arraysize(left) - 1, \
        middle, \
        SDL_arraysize(right) - 1, \
        sizeof(middle) \
    )

#ifndef __has_attribute
#   define __has_attribute(x) 0
#endif

#if __has_attribute(__fallthrough__)
#   define VVV_fallthrough __attribute__((__fallthrough__))
#else
#   define VVV_fallthrough do { } while (false) /* fallthrough */
#endif

#define MAYBE_FAIL(expr) \
    do \
    { \
        if (!expr) \
        { \
            goto fail; \
        } \
    } \
    while (false)

/* Positive modulo, because C/C++'s modulo operator sucks and is negative given
 * negative divisors.
 * WARNING! This double- and triple- evaluates. */
#define POS_MOD(a, b) (((a) % (b) + (b)) % (b))

//helperClass
class UtilityClass
{
public:
    UtilityClass(void);

    static std::string String(int _v);

    static int Int(const char* str, int fallback = 0);

    static std::string GCString(const std::vector<SDL_GameControllerButton>& buttons);

    std::string twodigits(int t);

    std::string timestring(int t);

    std::string number_words(int _t);


    static bool intersects( SDL_Rect A, SDL_Rect B );

    void updateglow(void);

    int glow;
    int slowsine;
    int glowdir;
};

#ifndef HELP_DEFINITION
extern UtilityClass help;
#endif

#endif /* UTILITYCLASS_H */
