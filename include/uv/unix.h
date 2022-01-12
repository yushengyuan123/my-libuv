//
// Created by 余圣源 on 2022/1/12.
//

#ifndef MY_LIBUV_UNIX_H
#define MY_LIBUV_UNIX_H

#define UV_TIMER_PRIVATE_FIELDS                                               \
    uv_timer_cb timer_cb;                                                       \
    void* heap_node[3];                                                         \
    uint64_t timeout;                                                           \
    uint64_t start_id;

#endif //MY_LIBUV_UNIX_H
