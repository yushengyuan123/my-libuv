//
// Created by 余圣源 on 2022/1/12.
//

#ifndef MY_LIBUV_UV_COMMON_H
#define MY_LIBUV_UV_COMMON_H

#define uv__has_active_handles(loop) ((loop)->active_handles > 0)

#define uv__has_active_reqs(loop) ((loop)->active_reqs.count > 0)

#endif //MY_LIBUV_UV_COMMON_H
