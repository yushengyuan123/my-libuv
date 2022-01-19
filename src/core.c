#include <stdio.h>
#include "stdlib.h"
#include "../include/uv.h"
#include "internal.h"
#include "uv-common.h"
#include "queue.h"
// todo 了解一下这个poll.h是干什么的
#include "poll.h"
#include "assert.h"
#include "limits.h"

static int uv__loop_alive(const uv_loop_t* loop) {
    return uv__has_active_handles(loop) ||
    uv__has_active_reqs(loop) ||
    loop->closing_handles != NULL;
}
// todo 有一个问题这个loop->pending_queue存放的是什么东西，你就知道pending时期是做什么事情了
// pending_queue队列是在loop init过程中uv__io_init函数中进行了厨师阿虎
static int uv__run_pending(uv_loop_t* loop) {
    QUEUE* q;
    QUEUE pq;
    uv__io_t* w;

    if (QUEUE_EMPTY(&loop->pending_queue)) {
        return 0;
    }
    // 把pending_queue的东西移动到pq队列
	QUEUE_MOVE(&loop->pending_queue, &pq);
    
    // 循环检查pq，逐个元素提取出来，把每一个node作为一个新的队列的头节点,同时执行
    while (!QUEUE_EMPTY(&pq)) {
    	q = QUEUE_HEAD(&pq);
    	QUEUE_REMOVE(q);
    	QUEUE_INIT(q);
    	w = QUEUE_DATA(q, uv__io_t, pending_queue);
    	w->cb(loop, w, POLLOUT);
    }
    
	return 1;
}

int uv_backend_timeout(const uv_loop_t* loop) {
	// 时间循环被外部停止了，所以让 `uv__io_poll` 理解返回
	// 以便尽快结束事件循环
	if (loop->stop_flag != 0)
		return 0;

	// 没有待处理的 handle 和 request，则也不需要等待了，同样让 `uv__io_poll`
	// 尽快返回
	// 这两个东西好像在handle或者req任务init的时候会递增
	// 例如fs中，mkdir任务在uv__req_register函数中就会把这个值增加
	if (!uv__has_active_handles(loop) && !uv__has_active_reqs(loop))
		return 0;

	// idle 队列不为空，也要求 `uv__io_poll` 尽快返回，这样尽快进入下一个时间循环
	// 否则会导致 idle 产生过高的延迟
	if (!QUEUE_EMPTY(&loop->idle_handles))
		return 0;

	// 和上一步目的一样，不过这里是换成了 pending 队列
	if (!QUEUE_EMPTY(&loop->pending_queue))
		return 0;

	// 和上一步目的一样，不过这里换成，待关闭的 handles，都是为了避免目标队列产生
	// 过高的延迟
	if (loop->closing_handles)
		return 0;

	// 这个next主要的目的就是检查timers有没有超时任务，没有的话就无限阻塞，有的话要立刻
	// 返回结束本次事件循环，以完成这些timers
	return uv__next_timeout(loop);
}

static unsigned int next_power_of_two(unsigned int val) {
	val -= 1;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	val += 1;
	return val;
}

// 入参 (loop, w->fd + 1)
static void maybe_resize(uv_loop_t* loop, unsigned int len) {
	uv__io_t** watchers;
	void* fake_watcher_list;
	void* fake_watcher_count;
	unsigned int nwatchers;
	unsigned int i;

	// 此时fd + 1 = 1；nwatchers = 0；
	if (len <= loop->nwatchers)
		return;

	/* Preserve fake watcher list and count at the end of the watchers */
	// 到这里的时候loop->watchers只经过了初始化为null
	if (loop->watchers != NULL) {
		fake_watcher_list = loop->watchers[loop->nwatchers];
		fake_watcher_count = loop->watchers[loop->nwatchers + 1];
	} else {
		fake_watcher_list = NULL;
		fake_watcher_count = NULL;
	}
	// len + 2 = 3
	nwatchers = next_power_of_two(len + 2) - 2;
	// 给这个watchers分配空间
	watchers = uv__reallocf(loop->watchers,
							(nwatchers + 2) * sizeof(loop->watchers[0]));

	if (watchers == NULL)
		abort();
	for (i = loop->nwatchers; i < nwatchers; i++)
		watchers[i] = NULL;
	watchers[nwatchers] = fake_watcher_list;
	watchers[nwatchers + 1] = fake_watcher_count;

	loop->watchers = watchers;
	loop->nwatchers = nwatchers;
}

// async start传入参数(&loop->async_io_watcher, uv__async_io, pipefd[0])
// pipefd[0]在linux平台上是经过了虚拟的能插入epoll的fd，uv__async_io是一个async的回调函数不是用户的
void uv__io_init(uv__io_t* w, uv__io_cb cb, int fd) {
	assert(cb != NULL);
	assert(fd >= -1);
	
	QUEUE_INIT(&w->pending_queue);
	QUEUE_INIT(&w->watcher_queue);
	
	w->cb = cb;
	w->fd = fd;
	w->events = 0;
	w->pevents = 0;

#if defined(UV_HAVE_KQUEUE)
	w->rcount = 0;
	w->wcount = 0;
#endif /* defined(UV_HAVE_KQUEUE) */
}

// 入参(loop, &loop->async_io_watcher, POLLIN)
void uv__io_start(uv_loop_t* loop, uv__io_t* w, unsigned int events) {
	w->pevents |= events;
	// 这段代码应该是有作用的，我尝试删掉了这段代码总有访问错误
	maybe_resize(loop, w->fd + 1);
#if !defined(__sun)
	/* The event ports backend needs to rearm all file descriptors on each and
	 * every tick of the event loop but the other backends allow us to
	 * short-circuit here if the event mask is unchanged.
	 */
	if (w->events == w->pevents)
		return;
#endif
	// 可以翻到上面 `uv__io_poll` 的部分，会发现其中有遍历 `loop->watcher_queue`
	// 将其中的 fd 都加入到 epoll 实例中，以订阅它们的事件的动作
	// 这里就是把这个w->watcher_queue插入到loop->watcher_queue对了中，
	// 当我们loop run的时候就会检查到有东西了！！！！
	if (QUEUE_EMPTY(&w->watcher_queue))
		// 在io run中loop->watcher_queue会从这个数据结构中提取观察者执行，所以我们
		// 要把它加入到观察者队列中
		QUEUE_INSERT_TAIL(&loop->watcher_queue, &w->watcher_queue);
	
	// 将 fd 和对应的任务关联的操作，同样可以翻看上面的 `uv__io_poll`，当接收到事件
	// 通知后，会有从 `loop->watchers` 中根据 fd 取出任务并执行其完成回调的动作
	// 另外，根据 fd 确保 watcher 不会被重复添加
	if (loop->watchers[w->fd] == NULL) {
		loop->watchers[w->fd] = w;
		loop->nfds++;
	}
}

int uv_run(uv_loop_t *loop, uv_run_mode mode) {
    int timeout;
    int r;
    int ran_pending;

    r = uv__loop_alive(loop);

    if (!r) {
        uv__update_time(loop);
    }

    printf("%ld\n", loop->time);
    

    while (r != 0 && loop->stop_flag == 0) {
        uv__update_time(loop);

        uv__run_timers(loop);
        
        ran_pending = uv__run_pending(loop);
        // 这个prepare，check，idle都是同样的作用，检查各自的loop->name##_handles队列
        // 遍历这个队列把元素取出来，执行各自的回调函数
        // todo 有两个问题，第一：什么任务会进入到这个队列中，第二：为什么它开始先把loop->name##_handles移送到
        // todo 新的队列中，但是后面又重新插入到了loop->name##_handles队列中
	    uv__run_idle(loop);
	    uv__run_prepare(loop);
	    
	    timeout = 0;
	    if ((mode == UV_RUN_ONCE && !ran_pending) || mode == UV_RUN_DEFAULT) {
	    	// 如果是 -1 表示一直等到有事件产生
	    	// 如果是 0 则立即返回，包含调用时产生的事件
	    	// 如果是其余整数，则以 milliseconds 为单位，规约到未来某个系统时间片内
	    	// 这个uv_backend_timeout首先检查idle，prepare，check这些有没有任务，有的话立即
	    	// 终止本次循环，进入下一次循环去执行这些任务，否则就会去检查timers看timers是否有超时的任务
	    	// 这里如果没有timers任务就会一直阻塞等待
	    	timeout = uv_backend_timeout(loop);
	    }
	
	    uv__io_poll(loop, timeout);
    }

    return 0;
}
