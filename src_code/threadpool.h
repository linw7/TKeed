//
// Latest edit by TeeKee on 2017/7/26.
//


#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

typedef struct tk_task{
	void (*func)(void *);
	void *arg;
	struct tk_task *next;
}tk_task_t;

typedef struct threadpool{
	pthread_mutex_t lock;    // 互斥锁
	pthread_cond_t cond;    // 条件变量
	pthread_t *threads;    // 线程
	tk_task_t *head;    // 任务链表
	int thread_count; 
	int queue_size;
	int shutdown;
	int started;
}tk_threadpool_t;

typedef enum{
	tk_tp_invalid = -1,
	tk_tp_lock_fail = -2,
	tk_tp_already_shutdown = -3,
	tk_tp_cond_broadcast = -4,
	tk_tp_thread_fail = -5,
}tk_threadpool_error_t;

tk_threadpool_t *threadpool_init(int thread_num);
int threadpool_add(tk_threadpool_t *pool, void (*func)(void *), void *arg);
int threadpool_destroy(tk_threadpool_t *pool, int gracegul);

#endif
