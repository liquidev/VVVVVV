// RAM allocation utilities.

#ifndef RAM_H
#define RAM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *RAM_malloc(size_t bytes);

void RAM_free(void *ptr);

void *RAM_calloc(size_t nmemb, size_t size);

void *RAM_realloc(void *ptr, size_t size);

// Might not be precise due to fragmentation.
size_t RAM_totalAllocated();

#ifdef __cplusplus
}
#endif

#endif
