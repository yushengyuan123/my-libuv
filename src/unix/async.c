//
// Created by 余圣源 on 2022/1/18.
//
#include "stdio.h"
#include "stdlib.h"
#include "../internal.h"
#include "unistd.h"
#include "errno.h"
#include "../uv-common.h"
#include "assert.h"
#include "poll.h"

#if defined(__APPLE__)
	/**empty**/
#else
	#include <sys/epoll.h>
	#include <sys/eventfd.h>
#endif

static void uv__async_io(uv_loop_t* loop, uv__io_t* w, unsigned int events) {
	char buf[1024];
	ssize_t r;
	QUEUE queue;
	QUEUE* q;
	uv_async_t* h;

	assert(w == &loop->async_io_watcher);

	for (;;) {
		// 从这个fd中提取出buf去读，就是上面使用 `eventfd` 创建的虚拟 fd
		r = read(w->fd, buf, sizeof(buf));

		if (r == sizeof(buf))
			continue;

		if (r != -1)
			break;

		if (errno == EAGAIN || errno == EWOULDBLOCK)
			break;

		if (errno == EINTR)
			continue;

		abort();
	}

	QUEUE_MOVE(&loop->async_handles, &queue);
	while (!QUEUE_EMPTY(&queue)) {
		q = QUEUE_HEAD(&queue);
		h = QUEUE_DATA(q, uv_async_t, queue);

		QUEUE_REMOVE(q);
		QUEUE_INSERT_TAIL(&loop->async_handles, q);

//		if (0 == uv__async_spin(h))
//			continue;  /* Not pending. */
			
		if (h->async_cb == NULL)
			continue;

		// 这个async_cb就是那个uv__work_done函数,uv__work_done函数里面会执行done
		// 对应fs就是uv__fs_done函数，uv__fs_done函数又会执行我们的用户自定义函数，整一条含调函数流程就通了
		h->async_cb(h);
	}
}

static int uv__async_start(uv_loop_t* loop) {
	printf("进入额async start\n");
	int pipefd[2];
	int err;

	// 在loop init的时候这个fd是已经被初始化为-1了
	if (loop->async_io_watcher.fd != -1)
		return 0;

#ifdef __linux__
	// `eventfd` 可以创建一个 epoll 内部维护的 fd，该 fd 可以和其他真实的 fd（比如 socket fd）一样
	// 添加到 epoll 实例中，可以监听它的可读事件，也可以对其进行写入操作，因此就用户代码就可以借助这个
	// 看似虚拟的 fd 来实现的事件订阅了
	err = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (err < 0)
		return UV__ERR(errno);

	pipefd[0] = err;
	pipefd[1] = -1;
#else
	printf("当前环境是非linux环境，无法直接使用epoll\n");
//	err = uv__make_pipe(pipefd, UV_NONBLOCK_PIPE);
//	if (err < 0)
//		return err;
#endif
	// 可以看到这个init初始化了pending和watcher队列，uv__async_io
	// 这是一个回调函数
	uv__io_init(&loop->async_io_watcher, uv__async_io, pipefd[0]);
	uv__io_start(loop, &loop->async_io_watcher, POLLIN);
	loop->async_wfd = pipefd[1];

	return 0;
}

// async_cb这是一个回调函数
// 入参(loop, &loop->wq_async, uv__work_done)
int uv_async_init(uv_loop_t* loop, uv_async_t* handle, uv_async_cb async_cb) {
	printf("进入init\n");
	int err;
	// 在win平台中是没有这句话的
	err = uv__async_start(loop);
	if (err)
		return err;

	uv__handle_init(loop, (uv_handle_t*)handle, UV_ASYNC);
	// async_cb == uv__work_done
	handle->async_cb = async_cb;
	handle->pending = 0;

	QUEUE_INSERT_TAIL(&loop->async_handles, &handle->queue);
	uv__handle_start(handle);

	return 0;
}

// 在loop_init操作中会有对应的async的init操作
static void uv__async_send(uv_loop_t* loop) {
	const void* buf;
	ssize_t len;
	int fd;
	int r;

	buf = "";
	len = 1;
	fd = loop->async_wfd;
	

#if defined(__linux__)
	if (fd == -1) {
		static const uint64_t val = 1;
		buf = &val;
		len = sizeof(val);
		fd = loop->async_io_watcher.fd;  /* eventfd */
	}
#endif
	// 果然事件通知这一端就是往 `eventfd` 创建的虚拟 fd 写入数据
	// 剩下的就是交给 epoll 高效的事件调度机制唤醒事件订阅方就可以了
	// 这个通过在这个fd中不断的写入数据，从而唤醒了主线程事件循环的epoll苏醒了
	do
		// fd = loop->async_wfd;
		r = write(fd, buf, len);
	while (r == -1 && errno == EINTR);

	if (r == len)
		return;
	
	if (r == -1)
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;

	abort();
}

// 这个函数的作用就是通知主线程，并且想这个watcher_queue队列里面放东西，然后run那里
// 就可以从这个队列里面提取到东西
// 入参(&w->loop->wq_async)，这个handle在loop init的时候进行过了初始化
int uv_async_send(uv_async_t* handle) {
	if (ACCESS_ONCE(int, handle->pending) != 0)
		return 0;
	
	// todo 这个东西不知道是干什么的有点抽象的
	/* Tell the other thread we're busy with the handle. */
//	if (cmpxchgi(&handle->pending, 0, 1) != 0)
//		return 0;
	// 这个函数的作用就是向这个文件描述符fd = loop->async_wfd;不断去write东西
	// 那我猜测loop->async_wfd就是epoll监听的文件描述符
	// 从而出发epoll回调
	uv__async_send(handle->loop);
	
	/* Tell the other thread we're done. */
//	if (cmpxchgi(&handle->pending, 1, 2) != 1)
//		abort();

	return 0;
}
