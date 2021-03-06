#ifndef QUEUE_H_
#define QUEUE_H_

#include <stddef.h>

// 这个队列的数据结构有一点和我们认知的不同，他是使用一个数组存放prev和next指针，其中下标0是next指针，1是prev指针
typedef void *QUEUE[2];

#define QUEUE_NEXT(q)       (*(QUEUE **) &((*(q))[0]))
#define QUEUE_PREV(q)       (*(QUEUE **) &((*(q))[1]))
#define QUEUE_PREV_NEXT(q)  (QUEUE_NEXT(QUEUE_PREV(q)))
#define QUEUE_NEXT_PREV(q)  (QUEUE_PREV(QUEUE_NEXT(q)))

/* Public macros. */
#define QUEUE_DATA(ptr, type, field)                                          \
    ((type *) ((char *) (ptr) - offsetof(type, field)))


#define QUEUE_FOREACH(q, h)                                                   \
    for ((q) = QUEUE_NEXT(h); (q) != (h); (q) = QUEUE_NEXT(q))

#define QUEUE_EMPTY(q)                                                        \
    ((const QUEUE *) (q) == (const QUEUE *) QUEUE_NEXT(q))

#define QUEUE_HEAD(q)                                                         \
    (QUEUE_NEXT(q))

#define QUEUE_INIT(q)                                                         \
    do {                                                                        \
        QUEUE_NEXT(q) = (q);                                                      \
        QUEUE_PREV(q) = (q);                                                      \
    }                                                                           \
    while (0)

#define QUEUE_ADD(h, n)                                                       \
    do {                                                                        \
        QUEUE_PREV_NEXT(h) = QUEUE_NEXT(n);                                       \
        QUEUE_NEXT_PREV(n) = QUEUE_PREV(h);                                       \
        QUEUE_PREV(h) = QUEUE_PREV(n);                                            \
        QUEUE_PREV_NEXT(h) = (h);                                                 \
    }                                                                           \
    while (0)
// 把一个h队列，在节点q处剪开，原来的q处位置头节点变为n
#define QUEUE_SPLIT(h, q, n)                                                  \
    do {                                                                        \
        QUEUE_PREV(n) = QUEUE_PREV(h);                                            \
        QUEUE_PREV_NEXT(n) = (n);                                                 \
        QUEUE_NEXT(n) = (q);                                                      \
        QUEUE_PREV(h) = QUEUE_PREV(q);                                            \
        QUEUE_PREV_NEXT(h) = (h);                                                 \
        QUEUE_PREV(q) = (n);                                                      \
    }                                                                           \
    while (0)
// 拿到头节点，然后把头节点在h队列剪开，新的头节点变为n
#define QUEUE_MOVE(h, n)                                                      \
    do {                                                                        \
        if (QUEUE_EMPTY(h))                                                       \
	        QUEUE_INIT(n);                                                          \
	    else {                                                                    \
	        QUEUE* q = QUEUE_HEAD(h);                                               \
	        QUEUE_SPLIT(h, q, n);                                                   \
        }                                                                         \
    }                                                                           \
    while (0)

#define QUEUE_INSERT_HEAD(h, q)                                               \
    do {                                                                        \
        QUEUE_NEXT(q) = QUEUE_NEXT(h);                                            \
        QUEUE_PREV(q) = (h);                                                      \
        QUEUE_NEXT_PREV(q) = (q);                                                 \
        QUEUE_NEXT(h) = (q);                                                      \
    }                                                                           \
    while (0)

#define QUEUE_INSERT_TAIL(h, q)                                               \
    do {                                                                        \
        QUEUE_NEXT(q) = (h);                                                      \
        QUEUE_PREV(q) = QUEUE_PREV(h);                                            \
        QUEUE_PREV_NEXT(q) = (q);                                                 \
        QUEUE_PREV(h) = (q);                                                      \
    }                                                                           \
    while (0)

#define QUEUE_REMOVE(q)                                                       \
    do {                                                                        \
        QUEUE_PREV_NEXT(q) = QUEUE_NEXT(q);                                       \
        QUEUE_NEXT_PREV(q) = QUEUE_PREV(q);                                       \
    }                                                                           \
    while (0)

#endif