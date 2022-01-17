#include "stdio.h"
#include "stdlib.h"
#include "../../include/uv.h"
#include "sys/statvfs.h"
#include "../uv-common.h"
#include "assert.h"
#include <sys/stat.h>
#include "errno.h"

static void uv__fs_work(struct uv__work* w) {
    int retry_on_eintr;
    uv_fs_t* req;
    ssize_t r;

    req = container_of(w, uv_fs_t, work_req);
    retry_on_eintr = !(req->fs_type == UV_FS_CLOSE ||
            req->fs_type == UV_FS_READ);

    do {
        errno = 0;

#define X(type, action)                                                       \
    case UV_FS_ ## type:                                                        \
        r = action;                                                               \
        break;

        switch (req->fs_type) {
            X(MKDIR, mkdir(req->path, req->mode));
            default: abort();
        }
#undef X
    } while (r == -1 && errno == EINTR && retry_on_eintr);

    if (r == -1)
        req->result = UV__ERR(errno);
    else
        req->result = r;

    if (r == 0 && (req->fs_type == UV_FS_STAT ||
    req->fs_type == UV_FS_FSTAT ||
    req->fs_type == UV_FS_LSTAT)) {
        req->ptr = &req->statbuf;
    }
}


static void uv__fs_done(struct uv__work* w, int status) {
    uv_fs_t* req;

    req = container_of(w, uv_fs_t, work_req);
    uv__req_unregister(req->loop, req);

    if (status == 1) {
        assert(req->result == 0);
        req->result = 1;
    }

    req->cb(req);
}

#define INIT(subtype)                                                         \
    do {                                                                        \
        if (req == NULL)                                                          \
            return UV_EINVAL;                                                       \
        UV_REQ_INIT(req, UV_FS);                                              \
        req->fs_type = UV_FS_ ## subtype;                                         \
        req->result = 0;                                                      \
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
                return -1;                                                     \
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
    // 初始化req的一些状态，例如类型，req的loop，type等等
    // todo 这个mkdir暂时不知道是什么意思
    INIT(MKDIR);
    PATH;
    req->mode = mode;
    POST;
}