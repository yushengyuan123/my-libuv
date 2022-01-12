#include "stdint.h"

#ifndef UV_H
#define UV_H

struct uv__io_s;
struct uv_loop_s;

typedef struct uv_loop_s uv_loop_t;
typedef struct uv_handle_s uv_handle_t;

typedef enum {
    UV_RUN_DEFAULT = 0,
    UV_RUN_ONCE,
    UV_RUN_NOWAIT
} uv_run_mode;

typedef void (*uv__io_cb)(struct uv_loop_s* loop,
        struct uv__io_s* w,
                unsigned int events);

struct uv_handle_s {
    void* data;
    /* read-only */
    uv_loop_t* loop;
    /* private */
    void* handle_queue[2];
    union {
        int fd;
        void* reserved[4];
    } u;
    unsigned int flags;
};

struct uv_loop_s {
    void* data;
    unsigned int active_handles;
    void *wq[2];
    struct {
        void* min;
        unsigned int nelts;
    } timer_heap;
    union {
        void* unused;
        unsigned int count;
    } active_reqs;
    void* handle_queue[2];
    void* pending_queue[2];
    void* prepare_handles[2];
    void* check_handles[2];
    void* idle_handles[2];
    void* async_handles[2];
    void* watcher_queue[2];
    uv_handle_t* closing_handles;
    //    uv__io_t** watchers;
    unsigned int nwatchers;                                                     \
    unsigned int nfds;
    unsigned long flags;                                                        \
    int backend_fd;
    int async_wfd;
    uint64_t time;
    //    uv__io_t async_io_watcher;
    //    uv__io_t signal_io_watcher;
};

struct uv__io_s {
    uv__io_cb cb;
    void* pending_queue[2];
    void* watcher_queue[2];
    unsigned int pevents; /* Pending event mask i.e. mask at next tick. */
    unsigned int events;  /* Current event mask. */
    int fd;
};

typedef void (*uv__io_cb)(struct uv_loop_s* loop,
        struct uv__io_s* w,
                unsigned int events);

#define uv__io_s uv__io_t;

extern int uv_loop_init(uv_loop_t* loop);

extern uv_loop_t* uv_loop_default();

#endif