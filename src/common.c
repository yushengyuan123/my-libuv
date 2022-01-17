#include "stdlib.h"
#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "../include/uv.h"

typedef struct {
    uv_malloc_func local_malloc;
    uv_realloc_func local_realloc;
    uv_calloc_func local_calloc;
    uv_free_func local_free;
} uv__allocator_t;

static uv__allocator_t uv__allocator = {
    malloc,
    realloc,
    calloc,
    free,
};

void* uv__malloc(size_t size) {
    if (size > 0)
        return uv__allocator.local_malloc(size);
    return NULL;
}

char* uv__strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* m = uv__malloc(len);
    if (m == NULL)
        return NULL;
    return memcpy(m, s, len);
}

void uv__free(void* ptr) {
    int saved_errno;

    /* Libuv expects that free() does not clobber errno.  The system allocator
     * honors that assumption but custom allocators may not be so careful.
     */
    saved_errno = errno;
    uv__allocator.local_free(ptr);
    errno = saved_errno;
}