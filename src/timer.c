#include "../include/uv.h"
#include "heap-inl.h"
#include "stdio.h"
#include "stdint.h"
#include "uv-common.h"
#include "limits.h"

static struct heap *timer_heap(const uv_loop_t *loop) {
    return (struct heap *) &loop->timer_heap;
}

static int timer_less_than(const struct heap_node *ha,
                           const struct heap_node *hb) {
    const uv_timer_t *a;
    const uv_timer_t *b;

    a = container_of(ha, uv_timer_t, heap_node);
    b = container_of(hb, uv_timer_t, heap_node);

    if (a->timeout < b->timeout)
        return 1;
    if (b->timeout < a->timeout)
        return 0;

    /* Compare start_id when both have the same timeout. start_id is
     * allocated with loop->timer_counter in uv_timer_start().
     */
    return a->start_id < b->start_id;
}

int uv_timer_init(uv_loop_t *loop, uv_timer_t *handle) {
    uv__handle_init(loop, (uv_handle_t *) handle, UV_TIMER);
    handle->timer_cb = NULL;
    handle->timeout = 0;
//    handle->repeat = 0;
    return 0;
}

int uv_timer_stop(uv_timer_t *handle) {
    if (!uv__is_active(handle))
        return 0;

    heap_remove(timer_heap(handle->loop),
                (struct heap_node *) &handle->heap_node,
                timer_less_than);

    uv__handle_stop(handle);

    return 0;
}

int uv_timer_start(uv_timer_t *handle,
                   uv_timer_cb cb,
                   long int timeout,
                   uint64_t repeat) {
    uint64_t clamped_timeout;

    if (cb == NULL) {
        return -1;
    }

    if (uv__is_active(handle)) {
        uv_timer_stop(handle);
    }
    // handle->loop->time 这个loop的在init的时候就是指向了主loop的
    // 所以这个handle->loop->time就是当前loop的时间
    clamped_timeout = handle->loop->time + timeout;

    // 当我们这个timeout是负数的时候，if就会成立，就是超时的情况
    if (clamped_timeout < timeout) {
        clamped_timeout = (uint64_t) - 1;
    }

    handle->timer_cb = cb;
    handle->timeout = clamped_timeout;
    handle->start_id = handle->loop->timer_counter++;

    heap_insert(timer_heap(handle->loop), (struct heap_node *) &handle->heap_node,
            timer_less_than);
    // 设置 handle 的开始状态，就是设置handle结构体里面的一些东西
    uv__handle_start(handle);

    return 0;
}


void uv__run_timers(uv_loop_t *loop) {
    struct heap_node *heap_node;
    uv_timer_t *handle;

    for (;;) {
        heap_node = heap_min(timer_heap(loop));
        if (heap_node == NULL) {
            break;
        }

        handle = container_of(heap_node, uv_timer_t, heap_node);

        if (handle->timeout > loop->time) {
            break;
        }

        uv_timer_stop(handle);

        handle->timer_cb(handle);
    }
}

int uv__next_timeout(const uv_loop_t* loop) {
	const struct heap_node* heap_node;
	const uv_timer_t* handle;
	uint64_t diff;

	heap_node = heap_min(timer_heap(loop));
	// 如果没有 timer 待处理，则可以放心的 block 住，等待事件到达
	if (heap_node == NULL) {
		return -1; /* block indefinitely */
	}
	
	handle = container_of(heap_node, uv_timer_t, heap_node);
	// 有 timer，且 timer 已经到了要被执行的时间内，则需让 `uv__io_poll`
	// 尽快返回，以在下一个事件循环迭代内处理超时的 timer
	if (handle->timeout <= loop->time)
		return 0;
	// 没有 timer 超时，用最小超时间减去、当前的循环时间的差值，作为超时时间
	// 因为在为了这个差值时间内是没有 timer 超时的，所以可以放心 block 以等待
	// epoll 事件
	diff = handle->timeout - loop->time;
	if (diff > INT_MAX)
		diff = INT_MAX;

	return (int) diff;
}
