#include <stdio.h>
#include "../include/uv.h"
#include "internal.h"
#include "uv-common.h"

static int uv__loop_alive(const uv_loop_t* loop) {
    return uv__has_active_handles(loop) ||
    uv__has_active_reqs(loop) ||
    loop->closing_handles != NULL;
}

int uv_run(uv_loop_t *loop, uv_run_mode mode) {
    int timeout;
    int r;
    int ran_pending;

    r = uv__loop_alive(loop);

    if (!r) {
        uv__update_time(loop)
    }

    int count = 0;

    while (r != 0 && count < 3) {
        printf("操你\n");
        count++;
    }

}
