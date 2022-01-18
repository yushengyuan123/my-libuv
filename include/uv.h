
#ifndef UV_H
#define UV_H

#include "stdint.h"
#include "stdio.h"
#include "unistd.h"
#include <sys/types.h>
#include <semaphore.h>

typedef struct uv__io_s uv__io_t;
struct uv_loop_s;

typedef struct uv_loop_s uv_loop_t;
typedef struct uv_handle_s uv_handle_t;
typedef struct uv_fs_s uv_fs_t;
typedef struct uv_thread_options_s uv_thread_options_t;
typedef struct uv_idle_s uv_idle_t;
typedef struct uv_poll_s uv_poll_t;
typedef struct uv_check_s uv_check_t;
typedef struct uv_async_s uv_async_t;
typedef struct uv_prepare_s uv_prepare_t;

#ifndef UV_IO_PRIVATE_PLATFORM_FIELDS
# define UV_IO_PRIVATE_PLATFORM_FIELDS /* empty */
#endif


typedef enum {
    UV_RUN_DEFAULT = 0,
    UV_RUN_ONCE,
    UV_RUN_NOWAIT
} uv_run_mode;

//typedef enum {
//    UV_UNKNOWN_REQ = 0,
//    UV_REQ,
//    UV_CONNECT,
//    UV_WRITE,
//    UV_SHUTDOWN,
//    UV_UDP_SEND,
//    UV_FS,
//    UV_WORK,
//    UV_GETADDRINFO,
//    UV_GETNAMEINFO,
//    UV_REQ_TYPE_MAX,
//} uv_req_type;

enum {
    UV_EINVAL = 1
};

typedef enum {
    UV_THREAD_NO_FLAGS = 0x00,
    UV_THREAD_HAS_STACK_SIZE = 0x01
} uv_thread_create_flags;

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
typedef void (*uv_prepare_cb)(uv_prepare_t* handle);
typedef void (*uv_idle_cb)(uv_idle_t* handle);
typedef void (*uv_check_cb)(uv_check_t* handle);
typedef void (*uv_poll_cb)(uv_poll_t* handle, int status, int events);
typedef void (*uv_async_cb)(uv_async_t* handle);

#define UV_ASYNC_PRIVATE_FIELDS                                               \
	uv_async_cb async_cb;                                                       \
	void* queue[2];                                                             \
	int pending;                                                                \

// unix
#define UV_ONCE_INIT PTHREAD_ONCE_INIT

#ifndef UV_PLATFORM_SEM_T
// 信号量的数据类型为结构sem_t，它本质上是一个长整型的数
# define UV_PLATFORM_SEM_T sem_t
#endif


typedef int uv_file;
typedef struct uv_buf_t {
    char* base;
    size_t len;
} uv_buf_t;
typedef gid_t uv_gid_t;
typedef uid_t uv_uid_t;
typedef pthread_once_t uv_once_t;
typedef pthread_t uv_thread_t;
typedef pthread_mutex_t uv_mutex_t;
typedef pthread_rwlock_t uv_rwlock_t;
typedef UV_PLATFORM_SEM_T uv_sem_t;
typedef pthread_cond_t uv_cond_t;
typedef pthread_key_t uv_key_t;

#define UV_POLL_PRIVATE_FIELDS                                                \
	uv__io_t io_watcher;

#define UV_PREPARE_PRIVATE_FIELDS                                             \
	uv_prepare_cb prepare_cb;                                                   \
	void* queue[2];                                                             \

#define UV_CHECK_PRIVATE_FIELDS                                               \
	uv_check_cb check_cb;                                                       \
	void* queue[2];

#define UV_IDLE_PRIVATE_FIELDS                                                \
	uv_idle_cb idle_cb;                                                         \
	void* queue[2];                                                             \



struct uv__io_s {
    uv__io_cb cb;
    void* pending_queue[2];
    void* watcher_queue[2];
    unsigned int pevents; /* Pending event mask i.e. mask at next tick. */
    unsigned int events;  /* Current event mask. */
    int fd;
    UV_IO_PRIVATE_PLATFORM_FIELDS
};



// threadpool
struct uv__work {
    void (*work)(struct uv__work *w);
    void (*done)(struct uv__work *w, int status);
    struct uv_loop_s* loop;
    void* wq[2];
};

typedef struct {
    long tv_sec;
    long tv_nsec;
} uv_timespec_t;

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

#define UV_REQ_TYPE_MAP(XX)                                                   \
    XX(REQ, req)                                                                \
    XX(CONNECT, connect)                                                        \
    XX(WRITE, write)                                                            \
    XX(SHUTDOWN, shutdown)                                                      \
    XX(UDP_SEND, udp_send)                                                      \
    XX(FS, fs)                                                                  \
    XX(WORK, work)                                                              \
    XX(GETADDRINFO, getaddrinfo)                                                \
    XX(GETNAMEINFO, getnameinfo)                                                \
    XX(RANDOM, random)                                                          \

#define UV_REQ_TYPE_PRIVATE /* empty */

typedef enum {
    UV_UNKNOWN_REQ = 0,
#define XX(uc, lc) UV_##uc,
    UV_REQ_TYPE_MAP(XX)
#undef XX
    UV_REQ_TYPE_PRIVATE
    UV_REQ_TYPE_MAX
} uv_req_type;

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


struct uv_async_s {
	UV_HANDLE_FIELDS
	UV_ASYNC_PRIVATE_FIELDS
};

#define UV_REQ_PRIVATE_FIELDS  /* empty */

#define UV_REQ_FIELDS                                                         \
    /* public */                                                                \
    void* data;                                                               \
    /* read-only */                                                             \
    uv_req_type type;                                                           \
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
    ssize_t result;
    void *ptr;
    const char *path;
    uv_stat_t statbuf;
    UV_FS_PRIVATE_FIELDS
};


struct uv_check_s {
	UV_HANDLE_FIELDS
	UV_CHECK_PRIVATE_FIELDS
};

struct uv_poll_s {
	UV_HANDLE_FIELDS
	uv_poll_cb poll_cb;
	UV_POLL_PRIVATE_FIELDS
};

struct uv_idle_s {
	UV_HANDLE_FIELDS
	UV_IDLE_PRIVATE_FIELDS
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
    unsigned int nwatchers;
    unsigned int nfds;
    unsigned long flags;
    unsigned int stop_flag;
    int backend_fd;
    int async_wfd;
    long int time;
    uint64_t timer_counter;
    uv_mutex_t wq_mutex;
    uv_async_t wq_async;
    uv__io_t async_io_watcher;
    uv__io_t** watchers;
    //    uv__io_t signal_io_watcher;
};

struct uv_thread_options_s {
    unsigned int flags;
    size_t stack_size;
    /* More fields may be added at any time. */
};

struct uv_prepare_s {
	UV_HANDLE_FIELDS
	UV_PREPARE_PRIVATE_FIELDS
};

typedef void (*uv__io_cb)(struct uv_loop_s* loop,
        struct uv__io_s* w,
                unsigned int events);

typedef void* (*uv_malloc_func)(size_t size);
typedef void* (*uv_realloc_func)(void* ptr, size_t size);
typedef void* (*uv_calloc_func)(size_t count, size_t size);
typedef void (*uv_free_func)(void* ptr);


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
extern void uv_cond_wait(uv_cond_t* cond, uv_mutex_t* mutex);

extern int uv_sem_init(uv_sem_t* sem, unsigned int value);
extern void uv_sem_destroy(uv_sem_t* sem);
extern void uv_sem_wait(uv_sem_t* sem);
extern void uv_sem_post(uv_sem_t* sem);

extern int uv_mutex_init(uv_mutex_t* mutex);
extern void uv_mutex_lock(uv_mutex_t* mutex);
extern void uv_mutex_unlock(uv_mutex_t* mutex) ;

// thread
extern void uv_once(uv_once_t* guard, void (*callback)(void));
typedef void (*uv_thread_cb)(void* arg);
extern int uv_thread_create(uv_thread_t* tid, uv_thread_cb entry, void* arg);

//async
extern int uv_async_send(uv_async_t* async);
extern int uv_async_init(uv_loop_t* loop, uv_async_t* handle, uv_async_cb async_cb);

// fs
extern int uv_fs_mkdir(uv_loop_t *loop,uv_fs_t *req, const char *path, int mode, uv_fs_cb cb);


#endif