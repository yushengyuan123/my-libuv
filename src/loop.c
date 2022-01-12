#include <stdio.h>
#include "../include/uv.h"
#include "heap-inl.h"
#include "queue.h"
#include "internal.h"

static uv_loop_t default_loop_struct;
static uv_loop_t *default_loop_pointer;

int uv_loop_init(uv_loop_t *loop) {
    heap_init((struct heap*) &loop->timer_heap);

    QUEUE_INIT(&loop->wq);
    QUEUE_INIT(&loop->idle_handles);
    QUEUE_INIT(&loop->async_handles);
    QUEUE_INIT(&loop->check_handles);
    QUEUE_INIT(&loop->prepare_handles);
    QUEUE_INIT(&loop->handle_queue);

    loop->active_handles = 0;
    // todo 这里初始化是0，为了测试改成了1，记得改回来
    loop->active_reqs.count = 1;
    loop->nfds = 0;
//    loop->watchers = NULL;
    loop->nwatchers = 0;
    loop->timer_counter = 0;
    QUEUE_INIT(&loop->pending_queue);
    QUEUE_INIT(&loop->watcher_queue);

    uv__update_time(loop);
    loop->async_wfd = -1;

    return 0;
}

uv_loop_t* uv_loop_default() {
    if (default_loop_pointer != NULL) {
        return default_loop_pointer;
    }

    uv_loop_init(&default_loop_struct);

    default_loop_pointer = &default_loop_struct;

    return default_loop_pointer;
}

