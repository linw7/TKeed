//
// Latest edit by TeeKee on 2017/7/23.
//

#ifndef TK_TIMER_H
#define TK_TIMER_H

#include "priority_queue.h"
#include "http_request.h"

#define TIMEOUT_DEFAULT 500     /* ms */

// 函数指针，负责超时处理，tk_add_timer时指定处理函数
typedef int (*timer_handler_pt)(tk_http_request_t* request);

typedef struct tk_timer{
    size_t key;    // 标记超时时间
    int deleted;    // 标记是否被删除
    timer_handler_pt handler;    // 超时处理，add时指定
    tk_http_request_t* request;    // 指向对应的request请求
} tk_timer_t;

// tk_pq_t定义在"priority_queue.h"中，优先队列中节点
extern tk_pq_t tk_timer;
extern size_t tk_current_msec;

int tk_timer_init();
int tk_find_timer();
void tk_handle_expire_timers();
void tk_add_timer(tk_http_request_t* request, size_t timeout, timer_handler_pt handler);
void tk_del_timer(tk_http_request_t* request);
int timer_comp(void *ti, void *tj);

#endif