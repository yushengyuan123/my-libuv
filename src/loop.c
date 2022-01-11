#include <stdio.h>
#include "uv.h"
#include "heap-inl.h"

static uv_loop_t default_loop_pointer;

int uv_loop_init(uv_loop_t *loop) {
    heap_init((struct *) &loop->timer_heap);
}

uv_loop_t uv_loop_default() {
    if (default_loop_pointer != NULL) {
        return default_loop_pointer;
    }
    return uv_loop_init(default_loop_pointer);
}

