#include "stdio.h"
#include "stdlib.h"
#include "../include/uv.h"
#include "uv-common.h"
#include "pthread.h"
#include "queue.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MAX_THREADPOOL_SIZE 1024

static uv_once_t once = UV_ONCE_INIT;
static uv_thread_t default_threads[4];
static uv_cond_t cond;
static uv_mutex_t mutex;

static void worker(void* arg) {
    struct uv__work* w;
    QUEUE* q;
    int is_slow_work;

    uv_sem_post((uv_sem_t*) arg);
    arg = NULL;
    // `idle_threads` 和 `run_slow_work_message` 这些是线程共享的，所以要加个锁
    uv_mutex_lock(&mutex);
    for (;;) {
        /* `mutex` should always be locked at this point. */

        /* Keep waiting while either no work is present or only slow I/O
           and we're at the threshold for that. */
        // 这里的条件判断，可以大致看成是「没有任务」为 true
        while (QUEUE_EMPTY(&wq) ||
        (QUEUE_HEAD(&wq) == &run_slow_work_message &&
        QUEUE_NEXT(&run_slow_work_message) == &wq &&
        slow_io_work_running >= slow_work_thread_threshold())) {
            idle_threads += 1;
            // https://blog.csdn.net/zzran/article/details/8830213
            // 这个函数大致可以猜想到他是给线程分配任务的，因为后面空闲线程-1
            // 这个函数是调用系统内部的`pthread_cond_wait`，这个会自动释放锁
            uv_cond_wait(&cond, &mutex);
            idle_threads -= 1;
        }

        // 执行到了这里，说明线程已经被唤醒了，开始从队列中取出任务来做
        q = QUEUE_HEAD(&wq);
        // 收到了退出的信息，终止线程for循环，退出
        if (q == &exit_message) {
            uv_cond_signal(&cond);
            uv_mutex_unlock(&mutex);
            break;
        }
        // 任务完成了移除掉这个节点
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);  /* Signal uv_cancel() that the work req is executing. */

        is_slow_work = 0;
        if (q == &run_slow_work_message) {
            /* If we're at the slow I/O threshold, re-schedule until after all
               other work in the queue is done. */
            if (slow_io_work_running >= slow_work_thread_threshold()) {
                QUEUE_INSERT_TAIL(&wq, q);
                continue;
            }

            /* If we encountered a request to run slow I/O work but there is none
               to run, that means it's cancelled => Start over. */
            if (QUEUE_EMPTY(&slow_io_pending_wq))
                continue;

            is_slow_work = 1;
            slow_io_work_running++;

            q = QUEUE_HEAD(&slow_io_pending_wq);
            QUEUE_REMOVE(q);
            QUEUE_INIT(q);

            /* If there is more slow I/O work, schedule it to be run as well. */
            if (!QUEUE_EMPTY(&slow_io_pending_wq)) {
                QUEUE_INSERT_TAIL(&wq, &run_slow_work_message);
                if (idle_threads > 0)
                    uv_cond_signal(&cond);
            }
        }
        // 注意这个锁是针对对队列操作上的锁，因为很多线程都希望对这个队列进行操作
        // 这个锁并不是要执行任务的，不要搞错了，上面已经弹出队列了，所以这里可以放
        // 锁，给其他的线程对这个队列进行操作
        uv_mutex_unlock(&mutex);

        w = QUEUE_DATA(q, struct uv__work, wq);
        w->work(w);
        // 下面要操作 `w->loop->wq` 所以要上锁，不知道其实这个东西是操作什么的
        uv_mutex_lock(&w->loop->wq_mutex);
        w->work = NULL;  /* Signal uv_cancel() that the work req is done
                        executing. */
        // 这里我猜想就是把任务里面的东西，放入到这个loop的结构题里面，方便后续主线程的调用
        QUEUE_INSERT_TAIL(&w->loop->wq, &w->wq);
        // 唤醒主线程的事件循环
        uv_async_send(&w->loop->wq_async);
        uv_mutex_unlock(&w->loop->wq_mutex);

        /* Lock `mutex` since that is expected at the start of the next
         * iteration. */
        uv_mutex_lock(&mutex);
        if (is_slow_work) {
            /* `slow_io_work_running` is protected by `mutex`. */
            slow_io_work_running--;
        }
    }
}

// 线程池的初始化
static void init_threads(void) {
    unsigned int i;
    const char* val;
    uv_sem_t sem;

    nthreads = ARRAY_SIZE(default_threads);
    // 根据用户的环境变量修改创建的线程池个数
    val = getenv("UV_THREADPOOL_SIZE");
    if (val != NULL)
        nthreads = atoi(val);
    if (nthreads == 0)
        nthreads = 1;
    if (nthreads > MAX_THREADPOOL_SIZE)
        nthreads = MAX_THREADPOOL_SIZE;

    // static uv_thread_t default_threads[4];
    threads = default_threads;
    if (nthreads > ARRAY_SIZE(default_threads)) {
        threads = uv__malloc(nthreads * sizeof(threads[0]));
        if (threads == NULL) {
            nthreads = ARRAY_SIZE(default_threads);
            threads = default_threads;
        }
    }

    // 初始化线程池状态
    if (uv_cond_init(&cond))
        abort();

    // 初始化锁的状态
    if (uv_mutex_init(&mutex))
        abort();
    // static QUEUE wq;wq是一个全局的队列
    QUEUE_INIT(&wq);
    QUEUE_INIT(&slow_io_pending_wq);
    QUEUE_INIT(&run_slow_work_message);

    if (uv_sem_init(&sem, 0))
        abort();

    for (i = 0; i < nthreads; i++)
        // 创建线程池，他是线程池的执行内容
        if (uv_thread_create(threads + i, worker, &sem))
            abort();

        for (i = 0; i < nthreads; i++)
            uv_sem_wait(&sem);

        uv_sem_destroy(&sem);
}

static void init_once(void) {
    init_threads();
}

void uv__work_submit(uv_loop_t *loop,
                     struct uv__work *w,
                     enum uv__work_kind kind,
                     void (*work)(struct uv__work* w),
                     void (*done)(struct uv__work* w, int status)) {
    pthread_once(&once, init_once);
}