#include "stdio.h"
#include "heap-inl.h"
#include "stdlib.h"
#include "time.h"
#include "../include/uv.h"

int repeat = 0;
int repeatCount = 3;

static void callback(uv_timer_t *handle) {
    repeat = repeat + 1;
    printf("cur %d\n", repeat);
    if (repeatCount == repeat) {
        uv_timer_stop(handle);
    }
}

int main() {
   uv_loop_t *loop = uv_loop_default();
   uv_timer_t handle;

   uv_timer_init(loop, &handle);
   uv_timer_start(&handle, callback, -1, 0);

   uv_run(loop, UV_RUN_DEFAULT);
}