//
// Latest edit by TeeKee on 2017/7/23.
//


#include <sys/time.h>
#include "timer.h"

tk_pq_t tk_timer;
size_t tk_current_msec;

int timer_comp(void* ti, void* tj){
    tk_timer_t* timeri = (tk_timer_t*)ti;
    tk_timer_t* timerj = (tk_timer_t*)tj;
    return (timeri->key < timerj->key) ? 1 : 0;
}

void tk_time_update(){
    // 获取当前时间
    struct timeval tv;
    int rc = gettimeofday(&tv, NULL);
    tk_current_msec = ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

int tk_timer_init(){
    // 建立连接后立即初始化
    // 初始优先队列大小TK_PQ_DEFAULT_SIZE = 10
    int rc = tk_pq_init(&tk_timer, timer_comp, TK_PQ_DEFAULT_SIZE);

    // 更新当前时间
    tk_time_update();
    return 0;
}

int tk_find_timer(){
    int time;
    // 返回队列中最早时间和当前时间之差
    while(!tk_pq_is_empty(&tk_timer)){
        // 更新当前时间
        tk_time_update();
        // timer_node指向最小的时间
        tk_timer_t* timer_node = (tk_timer_t*)tk_pq_min(&tk_timer);
        // 如果已删则释放此节点（tk_del_timer只置位不删除）
        if(timer_node->deleted){
            int rc = tk_pq_delmin(&tk_timer);
            free(timer_node);
            continue;
        }
        // 此时timer_node为时间最小节点，time为优先队列里最小时间减去当前时间
        time = (int)(timer_node->key - tk_current_msec);
        time = (time > 0) ? time : 0;
        break;
    }
    return time;
}

void tk_handle_expire_timers(){
    while(!tk_pq_is_empty(&tk_timer)){
        // 更新当前时间
        tk_time_update();
        tk_timer_t* timer_node = (tk_timer_t*)tk_pq_min(&tk_timer);
        // 如果已删则释放此节点
        if(timer_node->deleted){
            int rc = tk_pq_delmin(&tk_timer); 
            free(timer_node);
            continue;
        }
        // 最早入队列节点超时时间大于当前时间（未超时）
        // 结束超时检查，顺带删了下标记为删除的节点
        if(timer_node->key > tk_current_msec){
            return;
        }
        // 出现了没被删但是超时的情况，调用handler处理
        if(timer_node->handler){
            timer_node->handler(timer_node->request);
        }
        int rc = tk_pq_delmin(&tk_timer); 
        free(timer_node);
    }
}

void tk_add_timer(tk_http_request_t* request, size_t timeout, timer_handler_pt handler){
    tk_time_update();
    // 申请新的tk_timer_t节点，并加入到tk_http_request_t的timer下
    tk_timer_t* timer_node = (tk_timer_t*)malloc(sizeof(tk_timer_t));
    request->timer = timer_node;
    // 加入时设置超时阈值，删除信息等
    timer_node->key = tk_current_msec + timeout;
    timer_node->deleted = 0;
    timer_node->handler = handler;
    // 注意需要在tk_timer_t节点中反向设置指向对应resquest的指针
    timer_node->request = request;
    // 将新节点插入优先队列
    int rc = tk_pq_insert(&tk_timer, timer_node);
}

void tk_del_timer(tk_http_request_t* request) {
    tk_time_update();
    tk_timer_t* timer_node = request->timer;
    // 惰性删除
    // 标记为已删，在find_timer和handle_expire_timers检查队列时会删除
    timer_node->deleted = 1;
}