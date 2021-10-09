#include <SDL2/SDL.h>

#include "RAM.h"

/* Handle third-party dependencies' needs here */

void* lodepng_malloc(size_t size)
{
    return RAM_malloc(size);
}

void* lodepng_realloc(void* ptr, size_t new_size)
{
    return RAM_realloc(ptr, new_size);
}

void lodepng_free(void* ptr)
{
    RAM_free(ptr);
}
