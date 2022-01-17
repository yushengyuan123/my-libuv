#include "stdio.h"
#include "pthread.h"
#include "uv-common.h"
#include "../include/uv.h"
#include "stdlib.h"
#include <sys/resource.h>
#include <unistd.h>
#include "limits.h"
#include "sys/sem.h"
#include "sys/semaphore.h"
#include "errno.h"

#define platform_needs_custom_semaphore 1

typedef struct uv_semaphore_s {
    uv_mutex_t mutex;
    uv_cond_t cond;
    unsigned int value;
} uv_semaphore_t;


int uv_mutex_init(uv_mutex_t* mutex) {
    return UV__ERR(pthread_mutex_init(mutex, NULL));
}

void uv_mutex_destroy(uv_mutex_t* mutex) {
    if (pthread_mutex_destroy(mutex))
        abort();
}

void uv_once(uv_once_t* guard, void (*callback)(void)) {
    if (pthread_once(guard, callback))
        abort();
}

int uv_cond_init(uv_cond_t* cond) {
    return UV__ERR(pthread_cond_init(cond, NULL));
}

void uv_cond_signal(uv_cond_t* cond) {
    if (pthread_cond_signal(cond))
        abort();
}

void uv_cond_destroy(uv_cond_t* cond) {
#if defined(__APPLE__) && defined(__MACH__)
    /* It has been reported that destroying condition variables that have been
     * signalled but not waited on can sometimes result in application crashes.
     * See https://codereview.chromium.org/1323293005.
     */

    pthread_mutex_t mutex;
    struct timespec ts;
    int err;

    if (pthread_mutex_init(&mutex, NULL))
        abort();

    if (pthread_mutex_lock(&mutex))
        abort();

    ts.tv_sec = 0;
    ts.tv_nsec = 1;

    // todo 这里在没有修改worker前是有问题的
    err = pthread_cond_timedwait_relative_np(cond, &mutex, &ts);

    if (err != 0 && err != ETIMEDOUT) {
        abort();
    }

    if (pthread_mutex_unlock(&mutex))
        abort();

    if (pthread_mutex_destroy(&mutex))
        abort();
#endif /* defined(__APPLE__) && defined(__MACH__) */
    if (pthread_cond_destroy(cond))
        abort();
}

void uv_cond_broadcast(uv_cond_t* cond) {
    if (pthread_cond_broadcast(cond))
        abort();
}

void uv_cond_wait(uv_cond_t* cond, uv_mutex_t* mutex) {
    if (pthread_cond_wait(cond, mutex))
        abort();
}

size_t uv__thread_stack_size(void) {
#if defined(__APPLE__) || defined(__linux__)
    struct rlimit lim;

    /* getrlimit() can fail on some aarch64 systems due to a glibc bug where
     * the system call wrapper invokes the wrong system call. Don't treat
     * that as fatal, just use the default stack size instead.
     */
    if (0 == getrlimit(RLIMIT_STACK, &lim) && lim.rlim_cur != RLIM_INFINITY) {
        /* pthread_attr_setstacksize() expects page-aligned values. */
        lim.rlim_cur -= lim.rlim_cur % (rlim_t) getpagesize();

        /* Musl's PTHREAD_STACK_MIN is 2 KB on all architectures, which is
         * too small to safely receive signals on.
         *
         * Musl's PTHREAD_STACK_MIN + MINSIGSTKSZ == 8192 on arm64 (which has
         * the largest MINSIGSTKSZ of the architectures that musl supports) so
         * let's use that as a lower bound.
         *
         * We use a hardcoded value because PTHREAD_STACK_MIN + MINSIGSTKSZ
         * is between 28 and 133 KB when compiling against glibc, depending
         * on the architecture.
         */
        if (lim.rlim_cur >= 8192)
            if (lim.rlim_cur >= PTHREAD_STACK_MIN)
                return lim.rlim_cur;
    }
#endif

#if !defined(__linux__)
    return 0;
#elif defined(__PPC__) || defined(__ppc__) || defined(__powerpc__)
    return 4 << 20;  /* glibc default. */
#else
return 2 << 20;  /* glibc default. */
#endif
}

// uv_thread_create(threads + i = tid, worker = entry, &sem = arg)
int uv_thread_create_ex(uv_thread_t* tid,
                        const uv_thread_options_t* params,
                        void (*entry)(void *arg),
                        void *arg) {
    int err;
    pthread_attr_t* attr;
    pthread_attr_t attr_storage;
    size_t pagesize;
    size_t stack_size;

    /* Used to squelch a -Wcast-function-type warning. */
    union {
        void (*in)(void*);
        void* (*out)(void*);
    } f;

    stack_size =
            params->flags & UV_THREAD_HAS_STACK_SIZE ? params->stack_size : 0;

    attr = NULL;
    if (stack_size == 0) {
        stack_size = uv__thread_stack_size();
    } else {
        pagesize = (size_t)getpagesize();
        /* Round up to the nearest page boundary. */
        stack_size = (stack_size + pagesize - 1) &~ (pagesize - 1);
#ifdef PTHREAD_STACK_MIN
        if (stack_size < PTHREAD_STACK_MIN)
            stack_size = PTHREAD_STACK_MIN;
#endif
    }

    if (stack_size > 0) {
        attr = &attr_storage;

        if (pthread_attr_init(attr))
            abort();

        if (pthread_attr_setstacksize(attr, stack_size))
            abort();
    }

    f.in = entry;
//    　　第一个参数为指向线程标识符的指针。
//
//    　　第二个参数用来设置线程属性。
//
//    　　第三个参数是线程运行函数的地址。
//
//    　　最后一个参数是运行函数的参数。
    err = pthread_create(tid, attr, f.out, arg);

    if (attr != NULL)
        pthread_attr_destroy(attr);

    return UV__ERR(err);
}

static int uv__custom_sem_init(uv_sem_t* sem_, unsigned int value) {
    int err;
    uv_semaphore_t* sem;

    sem = uv__malloc(sizeof(*sem));
    // 这里应该是内存不足的意思，分配失败
    if (sem == NULL)
        return 1;

    if ((err = uv_mutex_init(&sem->mutex)) != 0) {
        uv__free(sem);
        return err;
    }

    if ((err = uv_cond_init(&sem->cond)) != 0) {
        uv_mutex_destroy(&sem->mutex);
        uv__free(sem);
        return err;
    }

    sem->value = value;
    *(uv_semaphore_t**)sem_ = sem;

    return 0;
}

static void uv__custom_sem_wait(uv_sem_t* sem_) {
    uv_semaphore_t* sem;

    sem = *(uv_semaphore_t**)sem_;
    uv_mutex_lock(&sem->mutex);
    while (sem->value == 0)
        uv_cond_wait(&sem->cond, &sem->mutex);
    sem->value--;
    uv_mutex_unlock(&sem->mutex);
}

static void uv__custom_sem_post(uv_sem_t* sem_) {
    uv_semaphore_t* sem;

    sem = *(uv_semaphore_t**)sem_;
    uv_mutex_lock(&sem->mutex);
    sem->value++;
    if (sem->value == 1)
        uv_cond_signal(&sem->cond);
    uv_mutex_unlock(&sem->mutex);
}

static void uv__custom_sem_destroy(uv_sem_t* sem_) {
    uv_semaphore_t* sem;

    sem = *(uv_semaphore_t**)sem_;

    uv_cond_destroy(&sem->cond);
    uv_mutex_destroy(&sem->mutex);
    uv__free(sem);
}

int uv_sem_init(uv_sem_t* sem, unsigned int value) {
    return uv__custom_sem_init(sem, value);
}

void uv_sem_post(uv_sem_t* sem) {
    uv__custom_sem_post(sem);
}

void uv_sem_wait(uv_sem_t* sem) {
    uv__custom_sem_wait(sem);
}

void uv_sem_destroy(uv_sem_t* sem) {
    uv__custom_sem_destroy(sem);
}

void uv_mutex_lock(uv_mutex_t* mutex) {
    if (pthread_mutex_lock(mutex))
        abort();
}

void uv_mutex_unlock(uv_mutex_t* mutex) {
    if (pthread_mutex_unlock(mutex))
        abort();
}

int uv_thread_create(uv_thread_t *tid, void (*entry)(void *arg), void *arg) {
    uv_thread_options_t params;
    params.flags = UV_THREAD_NO_FLAGS;
    return uv_thread_create_ex(tid, &params, entry, arg);
}
