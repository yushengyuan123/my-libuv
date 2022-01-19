#include <stdio.h>
#include "../include/uv.h"
#include "heap-inl.h"
#include "queue.h"
#include "internal.h"
#include "uv-common.h"

static uv_loop_t default_loop_struct;
static uv_loop_t *default_loop_pointer;

int uv_loop_init(uv_loop_t *loop) {
	int err;
	
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
    loop->watchers = NULL;
    loop->nwatchers = 0;
    loop->timer_counter = 0;
    QUEUE_INIT(&loop->pending_queue);
    QUEUE_INIT(&loop->watcher_queue);

    loop->closing_handles = NULL;
    uv__update_time(loop);
    loop->async_wfd = -1;
    loop->backend_fd = -1;
    loop->timer_counter = 0;
    loop->stop_flag = 0;
    loop->async_io_watcher.fd = -1;
    
    // 这个wq_mutex锁在线程的worker回调函数中被使用
    err = uv_mutex_init(&loop->wq_mutex);
    
    if (err) {
    	printf("loop->wq_mutex初始化失败\n");
    }
    
    // 总结一下这里做了什么事情。首先虚拟除了一个能够插入epoll的fd，调用uv__io_init，
    // 初始化pending_queue和watcher_queue队列，把这个fd塞入了loop->async_io_watcher中
    // loop->async_wfd = pipefd[1];。然后调用uv__io_start。检查loop->async_io_watcher
    // 中的watcher_queue队列，一个一个元素提取出来
    err = uv_async_init(loop, &loop->wq_async, uv__work_done);

    
    uv__free(loop->watchers);
    loop->nwatchers = 0;
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

