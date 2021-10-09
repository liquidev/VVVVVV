#include "RAM.h"

#include <SDL2/SDL.h>

#include "Vlogging.h"

static size_t total = 0;

typedef struct AllocationHeader {
    size_t allocated_size;
} AllocationHeader;

// This doesn't work.

// static void *track(void *allocation, size_t bytes) {
//     AllocationHeader *ah = allocation;
//     ah->allocated_size = bytes;
//     total += bytes;
//     vlog_debug("(RAM) usage: %u    <++ %u", total, bytes);
//     return ah + 1;
// }

// static void untrack(void *allocation) {
//     AllocationHeader *ah = ((AllocationHeader *)(allocation)) - 1;
//     total -= ah->allocated_size;
//     vlog_debug("(RAM) usage: %u    <-- %u", total, ah->allocated_size);
// }

void *RAM_malloc(size_t bytes)
{
    void *alloc = SDL_malloc(bytes);
    return alloc; // track(alloc, bytes);
}

void RAM_free(void *ptr)
{
    if (!ptr) return;
    // untrack(ptr);
    SDL_free(ptr);
}

void *RAM_calloc(size_t nmemb, size_t size)
{
    size_t bytes = nmemb * size;
    void *alloc = SDL_calloc(1, bytes);
    return alloc; // track(alloc, bytes);
}

void *RAM_realloc(void *ptr, size_t size)
{
    void *alloc;
    // untrack(ptr);
    alloc = SDL_realloc(ptr, size);
    return alloc; // track(alloc, size);
}

size_t RAM_totalAllocated()
{
    return total;
}
