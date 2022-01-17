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
static QUEUE wq;
static QUEUE run_slow_work_message;
// slow_io操作的等待队列
static QUEUE slow_io_pending_wq;
static QUEUE exit_message;
static uv_thread_t* threads;
// 正在工作的slow_io任务
static unsigned int slow_io_work_running;
static unsigned int idle_threads;
static unsigned int nthreads;

static unsigned int slow_work_thread_threshold(void) {
    return (nthreads + 1) / 2;
}

// 这个worker的arg是一个锁，我们后续再来看
static void worker(void* arg) {
    struct uv__work* w;
    QUEUE* q;
    int is_slow_work;

    // 这里就是解开主线程的锁，让主线程能够执行完毕，否则祝线程就阻塞住了，后面的post就执行不了，work执行不了
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
        // 正在工作的slow_io任务
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
            // 推进第一个mkdir任务的时候这个会执行
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

        // 这个w->work原来是通过这句话把它拿出来的！！！！
        w = QUEUE_DATA(q, struct uv__work, wq);
        w->work(w);
        // 下面要操作 `w->loop->wq` 所以要上锁，不知道其实这个东西是操作什么的
        uv_mutex_lock(&w->loop->wq_mutex);
        w->work = NULL;  /* Signal uv_cancel() that the work req is done
                        executing. */
        // 这里我猜想就是把任务里面的东西，放入到这个loop的结构题里面，方便后续主线程的调用
        QUEUE_INSERT_TAIL(&w->loop->wq, &w->wq);
        // 唤醒主线程的事件循环
//        uv_async_send(&w->loop->wq_async);
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

// q是wq队列，fs调用中，kind是UV__WORK_FAST_IO类型的
// 这个q追索到fs上层他是一个&req->work_req req，具体这个work-req装什么还不知道
// post(&w->wq, kind);
static void post(QUEUE* q, enum uv__work_kind kind) {
    // 加锁
    uv_mutex_lock(&mutex);
    // fs 操作中会进入到这个逻辑
    if (kind == UV__WORK_SLOW_IO) {
        /* Insert into a separate queue. */
        // 插入到等待队列中
        QUEUE_INSERT_TAIL(&slow_io_pending_wq, q);
        // 将任务插入到 `wq` 这个线程共享的队列中
        if (!QUEUE_EMPTY(&run_slow_work_message)) {
            /* Running slow I/O tasks is already scheduled => Nothing to do here.
               The worker that runs said other task will schedule this one as well. */
            uv_mutex_unlock(&mutex);
            return;
        }
        // 这个时候这个q空指针数组就指向了run_slow_work_message这个队列
        // 效果就是q发生了操作的话，等同于run_slow_work_message发生了操作
        // 结合上面相当于目前这个slow_io_pending_wq队列的第一个元素就是这个run_slow_work_message
        q = &run_slow_work_message;
    }
    // 把这个q任务插入到wq的队尾，这个时候已经收到了任务，在work函数中，线程由于
    // 没有任务可做，都被wait发生了阻塞，这里就是来任务了，所以要唤醒线程让他们干活
    // 在worker函数中取出这个wq队列的东西里面就有worker
    // 结合上面相当于目前这个wq队列的第一个元素就是这个run_slow_work_message
    QUEUE_INSERT_TAIL(&wq, q);
    // 如果有空闲线程，则通知它们开始工作
    if (idle_threads > 0)
        // 唤醒wait的线程，这里就是回调到了worker函数
        uv_cond_signal(&cond);
    uv_mutex_unlock(&mutex);
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
    // static QUEUE wq;wq是一个全局的队列，后续的worker函数会从这个wq队列里面取出东西
    QUEUE_INIT(&wq);
    QUEUE_INIT(&slow_io_pending_wq);
    QUEUE_INIT(&run_slow_work_message);

    // 这个sem是另外的一个锁在局部函数里面通过指针传递给了sem这个变量
    // 他本质上是一个长整形的数字
    // 重要：这个sem锁的作用就是阻塞主线程不然它提前退出，否则你的work有时就会执行不到
    if (uv_sem_init(&sem, 0))
        abort();

    for (i = 0; i < nthreads; i++) {
        // 创建线程池，他是线程池的执行内容，这个sem含义是设置运行函数的参数，就是worker的参数
        if (uv_thread_create(threads + i, worker, &sem))
            abort();
    }

    // 这段代码的作用就是阻塞主线程不然他提前退出
    for (i = 0; i < nthreads; i++) {
        uv_sem_wait(&sem);
    }

    uv_sem_destroy(&sem);
}

static void init_once(void) {
    init_threads();
}

// w == req->work_req
// 有一个疑问里面需要用到w的wq，但是这个东西什么时候被初始化的呢
void uv__work_submit(uv_loop_t *loop,
                     struct uv__work *w,
                     enum uv__work_kind kind,
                     void (*work)(struct uv__work* w),
                     void (*done)(struct uv__work* w, int status)) {
    // init_once的时候就会初始化三个队列，其中包括wq
    uv_once(&once, init_once);
    w->loop = loop;
    // 这里完成了对work函数就是我们的文件操作这个函数进行赋值，后续在这个worker函数中有作用
    w->work = work;
    w->done = done;
    // w->wq == void* wq[2] &w->wq含义就是拿出第一个元素
    // post函数的作用就是上传任务的
    post(&w->wq, kind);
}