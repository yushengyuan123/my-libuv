#include "stdio.h"
#include "../../include/uv.h"
#include "sys/statvfs.h"
#include "../uv-common.h"

#define INIT(subtype)                                                         \
    do {                                                                        \
        if (req == NULL)                                                          \
            return UV_EINVAL;                                                       \
        UV_REQ_INIT(req, UV_FS);                                                  \
        req->ptr = NULL;                                                          \
        req->loop = loop;                                                         \
        req->path = NULL;                                                         \
        req->new_path = NULL;                                                     \
        req->bufs = NULL;                                                         \
        req->cb = cb;                                                             \
    }                                                                           \
    while (0)

#define PATH                                                                  \
    do {                                                                        \
        if (cb == NULL) {                                                         \
            req->path = path;                                                       \
        } else {                                                                  \
            req->path = uv__strdup(path);                                           \
        if (req->path == NULL)                                                  \
            return UV_ENOMEM;                                                     \
        }                                                                         \
    }                                                                           \
    while (0)

#define POST                                                                  \
do {                                                                        \
    if (cb != NULL) {                                                         \
        uv__req_register(loop, req);                                            \
        uv__work_submit(loop,                                                   \
        &req->work_req,                                         \
        UV__WORK_FAST_IO,                                       \
        uv__fs_work,                                            \
        uv__fs_done);                                           \
        return 0;                                                               \
    }                                                                         \
    else {                                                                    \
        uv__fs_work(&req->work_req);                                            \
        return req->result;                                                     \
    }                                                                         \
}                                                                           \
while (0)

int uv_fs_mkdir(uv_loop_t *loop,
                uv_fs_t *req,
                const char *path,
                int mode,
                uv_fs_cb cb) {
    INIT(MKDIR);
    PATH;
    req->mode = mode;
    POST;
}