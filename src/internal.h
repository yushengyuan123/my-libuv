#ifndef UV_UNIX_INTERNAL_H_
#define UV_UNIX_INTERNAL_H_

#include "../include/uv.h"
#include "stdint.h"
#include "sys/time.h"
#include "stdio.h"

#define ACCESS_ONCE(type, var)                                                \
	(*(volatile type*) &(var))

#ifdef POLLRDHUP
# define UV__POLLRDHUP POLLRDHUP
#else
# define UV__POLLRDHUP 0x2000
#endif

#ifdef POLLPRI
# define UV__POLLPRI POLLPRI
#else
# define UV__POLLPRI 0
#endif

typedef enum {
    UV_CLOCK_PRECISE = 0,  /* Use the highest resolution clock available. */
    UV_CLOCK_FAST = 1      /* Use the fastest clock with <= 1ms granularity. */
} uv_clocktype_t;

/* core */
void uv__io_poll(uv_loop_t* loop, int timeout);
void uv__io_init(uv__io_t* w, uv__io_cb cb, int fd);
void uv__io_start(uv_loop_t* loop, uv__io_t* w, unsigned int events);

/* loop */
void uv__run_idle(uv_loop_t* loop);
void uv__run_check(uv_loop_t* loop);
void uv__run_prepare(uv_loop_t* loop);

int uv__make_pipe(int fds[2], int flags);


static long int uv__hrtime(uv_clocktype_t type) {
    struct timeval current;
    gettimeofday(&current, NULL);
    return current.tv_sec * 1000;
}


static void uv__update_time(uv_loop_t* loop) {
    /* Use a fast time source if available.  We only need millisecond precision.
     */
    loop->time = uv__hrtime(UV_CLOCK_FAST);
}

#endif