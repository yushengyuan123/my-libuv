#include "stdio.h"
#include "../../include/uv.h"
#include <sys/epoll.h>
#include "stdlib.h"
#include "assert.h"
#include "../queue.h"
#include "string.h"
#include "errno.h"
#include "../uv-common.h"
#include "poll.h"

//epoll_event结构体一般用在epoll机制中，其定义如下：
struct epoll_event{
	uint32_t events;   /* Epoll events */
	epoll_data_t data;    /* User data variable */
};

void uv_io_poll(uv_loop_t* loop, int timeout) {
	static const int max_safe_timeout = 1789569;
	static int no_epoll_pwait_cached;
	static int no_epoll_wait_cached;
	int no_epoll_pwait;
	int no_epoll_wait;
	struct epoll_event events[1024];
	struct epoll_event* pe;
	struct epoll_event e;
	int real_timeout;
	QUEUE* q;
	uv__io_t* w;
	sigset_t sigset;
	uint64_t sigmask;
	uint64_t base;
	int have_signals;
	int nevents;
	int count;
	int nfds;
	int fd;
	int op;
	int i;
	int user_timeout;
	int reset_timeout;
	
	// 这个nfds应该是指文件描述符的个数
	if (loop->nfds == 0) {
		assert(QUEUE_EMPTY(&loop->watcher_queue));
		return;
	}
	// 这个函数在socket中多用于清空数组.如:原型是memset(buffer, 0, sizeof(buffer))
	// Memset 用来对一段内存空间全部设置为某个字符，一般用在对定义的字符串进行初始化为‘ ’或‘/0’
	// 应该是清空结构体e的意思 https://blog.csdn.net/qq_27522735/article/details/53374765
	memset(&e, 0, sizeof(e));
	
	// todo 什么时候这个watcher_queue会有东西呢？
	// 这个watcher_queue是在loop init函数中，经过了一系列流程然后到达uv__io_init函数的时候被初始化了
	// 就是在loop init阶段就已经进行了初始化
	// init的时候会把w队列放入到loop->watcher_queue中，这个时候检查就会有东西了
	while (!QUEUE_EMPTY(&loop->watcher_queue)) {
		q = QUEUE_HEAD(&loop->watcher_queue);
		QUEUE_REMOVE(q);
		QUEUE_INIT(q);
		
		// 这个w存放了很多的内容，有事件注册的epoll fd，回调函数等等
		w = QUEUE_DATA(q, uv__io_t, watcher_queue);
		// todo 这个pevents暂时暂时不知道是什么东西
		assert(w->pevents != 0);
		assert(w->fd >= 0);
		assert(w->fd < (int) loop->nwatchers);
		
		e.events = w->pevents;
		// 把描述符加入到epoll中,在async中我们的fs操作通过eventfd，也搞出了一个fd
		// 就是这个w->fd，这里就是把这个fd加入到了epoll中
		e.data.fd = w->fd;
		
		if (w->events == 0) {
			op = EPOLL_CTL_ADD;
		} else {
			op = EPOLL_CTL_MOD;
		}
		// todo 有一个问题，这里是什么时候调用epoll_create，这个w->fd又是怎么来的
		// 从这个api的调用模式可以的看到这个backend_fd就是用来存放epoll创建出来的fd
		// 这句话的作用就是把我的任务虚拟出来的fd加入到了epoll中
		if (epoll_ctl(loop->backend_fd, op, w->fd, &e)) {
			if (errno != EEXIST)
				abort();

			assert(op == EPOLL_CTL_ADD);

			/* We've reactivated a file descriptor that's been watched before. */
			if (epoll_ctl(loop->backend_fd, EPOLL_CTL_MOD, w->fd, &e))
				abort();
		}
		
		w->events = w->pevents;
	}
	
//	sigmask = 0;
//	if (loop->flags & UV_LOOP_BLOCK_SIGPROF) {
//		sigemptyset(&sigset);
//		sigaddset(&sigset, SIGPROF);
//		sigmask |= 1 << (SIGPROF - 1);
//	}
	
	assert(timeout >= -1);
	base = loop->time;
	count = 48; /* Benchmarks suggest this gives the best throughput. */
	real_timeout = timeout;
	
	// todo 这里有一个疑问它似乎没有区分说自己是哪一个文件描述符的，然后执行对应的任务
	for (;;) {
		if (timeout != 0) {
			printf("uv__io_poll timeout != 0\n");
		}
		
		if (sizeof(int32_t) == sizeof(long) && timeout >= max_safe_timeout)
			timeout = max_safe_timeout;
		
		// 等待epoll事件返回，这个timeout是在外面算出来的，距离下一个timers事件发生的时间
		// 避免说这个epoll无限等待，当有定时任务超时了，能够即使终止epoll的事情去执行他们
		nfds = epoll_wait(loop->backend_fd,
						  events,
						  ARRAY_SIZE(events),
						  timeout);
		
		if (nfds == -1 && errno == ENOSYS) {
			printf("uv__run_poll uv__store_relaxed,挖坑\n");
		}
	}
	
	// 这个应该是有任务快要超时了还没来得及等待epoll的回调
	if (nfds == 0) {
		printf("uv__run_poll nfds == 0\n");
	}
	// 这个可能是epoll出现了异常
	if (nfds == -1) {
		printf("uv__run_poll nfds == -1\n");
	}
	
	have_signals = 0;
	nevents = 0;
	
	// 这里的逻辑就是有epoll回调了
	for (i = 0; i < nfds; i++) {
		// pe 是 epoll_events指针
		pe = events + i;
		fd = pe->data.fd;
		
		if (fd == -1) {
			continue;
		}
		
		assert(fd >= 0);
		assert((unsigned) fd < loop->nwatchers);
		
		// todo 这个东西是怎么来的
		w = loop->watchers[fd];
		
		if (w == NULL) {
			/* File descriptor that we've stopped watching, disarm it.
			 *
			 * Ignore all errors because we may be racing with another thread
			 * when the file descriptor is closed.
			 */
			epoll_ctl(loop->backend_fd, EPOLL_CTL_DEL, fd, pe);
			continue;
		}
		
		pe->events &= w->pevents | POLLERR | POLLHUP;
		
		if (pe->events == POLLERR || pe->events == POLLHUP)
			pe->events |= w->pevents & (POLLIN | POLLOUT | UV__POLLRDHUP | UV__POLLPRI);
		
		if (pe->events != 0) {
			/* Run signal watchers last.  This also affects child process watchers
			 * because those are implemented in terms of signal watchers.
			 */
			if (w == &loop->signal_io_watcher) {
				have_signals = 1;
			} else {
//				uv__metrics_update_idle_time(loop);
				// 执行回调函数，在fs的fd中对应的是uv__async_io函数，就是去读东西的read
				w->cb(loop, w, pe->events);
			}

			nevents++;
		}
	}
}