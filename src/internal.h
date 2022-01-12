#ifndef UV_UNIX_INTERNAL_H_
#define UV_UNIX_INTERNAL_H_

#include "../include/uv.h"
#include "stdint.h"
#include "sys/time.h"
#include "stdio.h"

typedef enum {
    UV_CLOCK_PRECISE = 0,  /* Use the highest resolution clock available. */
    UV_CLOCK_FAST = 1      /* Use the fastest clock with <= 1ms granularity. */
} uv_clocktype_t;


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