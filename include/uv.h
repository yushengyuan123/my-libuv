
#ifndef UV_H
#define UV_H

#include "stdint.h"
#include "stdio.h"
#include "unistd.h"
#include <sys/types.h>

struct uv__io_s;
struct uv_loop_s;

typedef struct uv_loop_s uv_loop_t;
typedef struct uv_handle_s uv_handle_t;
typedef struct uv_fs_s uv_fs_t;

typedef enum {
    UV_RUN_DEFAULT = 0,
    UV_RUN_ONCE,
    UV_RUN_NOWAIT
} uv_run_mode;

typedef enum {
    UV_UNKNOWN_REQ = 0,
    UV_REQ,
    UV_CONNECT,
    UV_WRITE,
    UV_SHUTDOWN,
    UV_UDP_SEND,
    UV_FS,
    UV_WORK,
    UV_GETADDRINFO,
    UV_GETNAMEINFO,
    UV_REQ_TYPE_MAX,
} uv_req_type;

enum {
    UV_EINVAL = 1
};

typedef enum {
    UV_FS_UNKNOWN = -1,
    UV_FS_CUSTOM,
    UV_FS_OPEN,
    UV_FS_CLOSE,
    UV_FS_READ,
    UV_FS_WRITE,
    UV_FS_SENDFILE,
    UV_FS_STAT,
    UV_FS_LSTAT,
    UV_FS_FSTAT,
    UV_FS_FTRUNCATE,
    UV_FS_UTIME,
    UV_FS_FUTIME,
    UV_FS_ACCESS,
    UV_FS_CHMOD,
    UV_FS_FCHMOD,
    UV_FS_FSYNC,
    UV_FS_FDATASYNC,
    UV_FS_UNLINK,
    UV_FS_RMDIR,
    UV_FS_MKDIR,
    UV_FS_MKDTEMP,
    UV_FS_RENAME,
    UV_FS_SCANDIR,
    UV_FS_LINK,
    UV_FS_SYMLINK,
    UV_FS_READLINK,
    UV_FS_CHOWN,
    UV_FS_FCHOWN,
    UV_FS_REALPATH,
    UV_FS_COPYFILE,
    UV_FS_LCHOWN,
    UV_FS_OPENDIR,
    UV_FS_READDIR,
    UV_FS_CLOSEDIR,
    UV_FS_STATFS,
    UV_FS_MKSTEMP,
    UV_FS_LUTIME
} uv_fs_type;

typedef void (*uv__io_cb)(struct uv_loop_s* loop,
        struct uv__io_s* w,
                unsigned int events);

typedef struct uv_timer_s uv_timer_t;
typedef void (*uv_timer_cb)(uv_timer_t* handle);
typedef void (*uv_fs_cb)(uv_fs_t* req);

// unix
typedef int uv_file;
typedef struct uv_buf_t {
    char* base;
    size_t len;
} uv_buf_t;
typedef gid_t uv_gid_t;
typedef uid_t uv_uid_t;
#define UV_ONCE_INIT PTHREAD_ONCE_INIT

// threadpool
struct uv__work {
    void (*work)(struct uv__work *w);
    void (*done)(struct uv__work *w, int status);
    struct uv_loop_s* loop;
    void* wq[2];
};


typedef struct {
    uint64_t st_dev;
    uint64_t st_mode;
    uint64_t st_nlink;
    uint64_t st_uid;
    uint64_t st_gid;
    uint64_t st_rdev;
    uint64_t st_ino;
    uint64_t st_size;
    uint64_t st_blksize;
    uint64_t st_blocks;
    uint64_t st_flags;
    uint64_t st_gen;
    uv_timespec_t st_atim;
    uv_timespec_t st_mtim;
    uv_timespec_t st_ctim;
    uv_timespec_t st_birthtim;
} uv_stat_t;

#define UV_HANDLE_PRIVATE_FIELDS                                              \
    uv_handle_t* next_closing;                                                  \
    unsigned int flags;

#define UV_HANDLE_FIELDS                                                      \
    /* public */                                                                \
    void* data;                                                                 \
    /* read-only */                                                             \
    uv_loop_t* loop;                                                            \
    /* private */                                                               \
    void* handle_queue[2];                                                      \
    union {                                                                     \
    int fd;                                                                   \
    void* reserved[4];                                                        \
    } u;                                                                        \
    UV_HANDLE_PRIVATE_FIELDS                                                  \

#define UV_REQ_PRIVATE_FIELDS  /* empty */

#define UV_REQ_FIELDS                                                         \
    /* public */                                                                \
    void* data;                                                                 \
    /* private */                                                               \
    void* reserved[6];                                                          \
    UV_REQ_PRIVATE_FIELDS                                                     \

#define UV_FS_PRIVATE_FIELDS                                                  \
    const char *new_path;                                                       \
    uv_file file;                                                               \
    int flags;                                                                  \
    mode_t mode;                                                                \
    unsigned int nbufs;                                                         \
    uv_buf_t* bufs;                                                             \
    off_t off;                                                                  \
    uv_uid_t uid;                                                               \
    uv_gid_t gid;                                                               \
    double atime;                                                               \
    double mtime;                                                               \
    struct uv__work work_req;                                                   \
    uv_buf_t bufsml[4];                                                           \

struct uv_timer_s {
    UV_HANDLE_FIELDS
    uv_timer_cb timer_cb;
    long int timeout;
    void* heap_node[3];
    uint64_t start_id;
};

struct uv_handle_s {
    UV_HANDLE_FIELDS
};

struct uv_fs_s {
    UV_REQ_FIELDS
    uv_fs_type fs_type;
    uv_loop_t *loop;
    uv_fs_cb cb;
    void *ptr;
    const char *path;
    uv_stat_t statbuf;
    UV_FS_PRIVATE_FIELDS
};

//struct uv_handle_s {
//    void* data;
//    /* read-only */
//    uv_loop_t* loop;
////    uv_handle_type type;
//    /* private */
//    void* handle_queue[2];
//    union {
//        int fd;
//        void* reserved[4];
//    } u;
//    unsigned int flags;
//    uv_handle_t* next_closing;
//};

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
    long int time;
    uint64_t timer_counter;
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
extern int uv_run(uv_loop_t *loop, uv_run_mode mode);
extern int uv_timer_init(uv_loop_t*, uv_timer_t* handle);
extern int uv_timer_start(uv_timer_t *handle,
                          uv_timer_cb cb,
                          long int timeout,
                          uint64_t repeat);
extern int uv_timer_stop(uv_timer_t *handle);
extern void uv__run_timers(uv_loop_t *loop);
extern int uv_cond_init(uv_cond_t* cond);
extern void uv_cond_destroy(uv_cond_t* cond);
extern void uv_cond_signal(uv_cond_t* cond);
extern void uv_cond_broadcast(uv_cond_t* cond);

#endif