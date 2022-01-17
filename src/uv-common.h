//
// Created by 余圣源 on 2022/1/12.
//

#ifndef MY_LIBUV_UV_COMMON_H
#define MY_LIBUV_UV_COMMON_H

#include <stddef.h>
#include "queue.h"
#include "../include/uv.h"

enum {
    /* Used by all handles. */
    UV_HANDLE_CLOSING                     = 0x00000001,
    UV_HANDLE_CLOSED                      = 0x00000002,
    UV_HANDLE_ACTIVE                      = 0x00000004,
    UV_HANDLE_REF                         = 0x00000008,
    UV_HANDLE_INTERNAL                    = 0x00000010,
    UV_HANDLE_ENDGAME_QUEUED              = 0x00000020,

    /* Used by streams. */
    UV_HANDLE_LISTENING                   = 0x00000040,
    UV_HANDLE_CONNECTION                  = 0x00000080,
    UV_HANDLE_SHUTTING                    = 0x00000100,
    UV_HANDLE_SHUT                        = 0x00000200,
    UV_HANDLE_READ_PARTIAL                = 0x00000400,
    UV_HANDLE_READ_EOF                    = 0x00000800,

    /* Used by streams and UDP handles. */
    UV_HANDLE_READING                     = 0x00001000,
    UV_HANDLE_BOUND                       = 0x00002000,
    UV_HANDLE_READABLE                    = 0x00004000,
    UV_HANDLE_WRITABLE                    = 0x00008000,
    UV_HANDLE_READ_PENDING                = 0x00010000,
    UV_HANDLE_SYNC_BYPASS_IOCP            = 0x00020000,
    UV_HANDLE_ZERO_READ                   = 0x00040000,
    UV_HANDLE_EMULATE_IOCP                = 0x00080000,
    UV_HANDLE_BLOCKING_WRITES             = 0x00100000,
    UV_HANDLE_CANCELLATION_PENDING        = 0x00200000,

    /* Used by uv_tcp_t and uv_udp_t handles */
    UV_HANDLE_IPV6                        = 0x00400000,

    /* Only used by uv_tcp_t handles. */
    UV_HANDLE_TCP_NODELAY                 = 0x01000000,
    UV_HANDLE_TCP_KEEPALIVE               = 0x02000000,
    UV_HANDLE_TCP_SINGLE_ACCEPT           = 0x04000000,
    UV_HANDLE_TCP_ACCEPT_STATE_CHANGING   = 0x08000000,
    UV_HANDLE_SHARED_TCP_SOCKET           = 0x10000000,

    /* Only used by uv_udp_t handles. */
    UV_HANDLE_UDP_PROCESSING              = 0x01000000,
    UV_HANDLE_UDP_CONNECTED               = 0x02000000,
    UV_HANDLE_UDP_RECVMMSG                = 0x04000000,

    /* Only used by uv_pipe_t handles. */
    UV_HANDLE_NON_OVERLAPPED_PIPE         = 0x01000000,
    UV_HANDLE_PIPESERVER                  = 0x02000000,

    /* Only used by uv_tty_t handles. */
    UV_HANDLE_TTY_READABLE                = 0x01000000,
    UV_HANDLE_TTY_RAW                     = 0x02000000,
    UV_HANDLE_TTY_SAVED_POSITION          = 0x04000000,
    UV_HANDLE_TTY_SAVED_ATTRIBUTES        = 0x08000000,

    /* Only used by uv_signal_t handles. */
    UV_SIGNAL_ONE_SHOT_DISPATCHED         = 0x01000000,
    UV_SIGNAL_ONE_SHOT                    = 0x02000000,

    /* Only used by uv_poll_t handles. */
    UV_HANDLE_POLL_SLOW                   = 0x01000000
};

enum {
    UV_TIMER = 14
};

enum uv__work_kind {
    UV__WORK_CPU,
    UV__WORK_FAST_IO,
    UV__WORK_SLOW_IO
};

# define UV__ERR(x) (-(x))

#define uv__has_active_handles(loop) ((loop)->active_handles > 0)

#define uv__req_register(loop, req)                                           \
    do {                                                                        \
        (loop)->active_reqs.count++;                                              \
    }                                                                           \
    while (0)

#define uv__req_unregister(loop, req)                                         \
    do {                                                                        \
        assert(uv__has_active_reqs(loop));                                        \
        (loop)->active_reqs.count--;                                              \
    }                                                                           \
    while (0)

#define uv__handle_platform_init(h) ((h)->next_closing = NULL)

# define UV_REQ_INIT(req, typ)                                                \
    do {                                                                        \
      (req)->type = (typ);                                                      \
    }                                                                           \
    while (0)

#define uv__req_register(loop, req)                                           \
    do {                                                                        \
        (loop)->active_reqs.count++;                                              \
    }                                                                           \
    while (0)


#define uv__active_handle_rm(h)                                               \
    do {                                                                        \
    (h)->loop->active_handles--;                                              \
    }                                                                           \
    while (0)

#define uv__active_handle_add(h)                                              \
    do {                                                                        \
        (h)->loop->active_handles++;                                              \
    }                                                                           \
    while (0)

#define uv__handle_stop(h)                                                    \
    do {                                                                        \
    if (((h)->flags & UV_HANDLE_ACTIVE) == 0) break;                          \
    (h)->flags &= ~UV_HANDLE_ACTIVE;                                          \
    if (((h)->flags & UV_HANDLE_REF) != 0) uv__active_handle_rm(h);           \
    }                                                                           \
    while (0)

#define uv__is_active(h)                                                      \
    (((h)->flags & UV_HANDLE_ACTIVE) != 0)

#define uv__is_closing(h)                                                     \
    (((h)->flags & (UV_HANDLE_CLOSING | UV_HANDLE_CLOSED)) != 0)

#define uv__handle_init(loop_, h, type_)                                      \
    do {                                                                        \
        (h)->loop = (loop_);                                                      \
        (h)->flags = UV_HANDLE_REF;  /* Ref the loop when active. */              \
        QUEUE_INSERT_TAIL(&(loop_)->handle_queue, &(h)->handle_queue);            \
        (h)->next_closing = NULL;                                             \
    }                                                                           \
while (0)

#define uv__handle_start(h)                                                   \
    do {                                                                        \
        if (((h)->flags & UV_HANDLE_ACTIVE) != 0) break;                          \
        (h)->flags |= UV_HANDLE_ACTIVE;                                           \
        if (((h)->flags & UV_HANDLE_REF) != 0) uv__active_handle_add(h);          \
        }                                                                           \
    while (0)

#define container_of(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))

//#define uv__is_active(h) (((h)->flags & UV_HANDLE_ACTIVE) != 0)

#define uv__has_active_reqs(loop) ((loop)->active_reqs.count > 0)

#endif //MY_LIBUV_UV_COMMON_H

void uv__work_submit(uv_loop_t* loop,
                     struct uv__work *w,
                     enum uv__work_kind kind,
                     void (*work)(struct uv__work *w),
                     void (*done)(struct uv__work *w, int status));

extern void* uv__malloc(size_t size);
extern void uv__free(void* ptr);
extern char *uv__strdup(const char* s);
