//
// Latest edit by TeeKee on 2017/7/23.
//

#include "epoll.h"

struct epoll_event* events;

int tk_epoll_create(int flags){
    int epoll_fd = epoll_create1(flags);
    if(epoll_fd == -1)
        return -1;
    events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    return epoll_fd;
}

// 注册新描述符
int tk_epoll_add(int epoll_fd, int fd, tk_http_request_t* request, int events){
    struct epoll_event event;
    event.data.ptr = (void*)request;
    event.events = events;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if(ret == -1)
        return -1;
}

// 修改描述符状态
int tk_epoll_mod(int epoll_fd, int fd, tk_http_request_t* request, int events){
    struct epoll_event event;
    event.data.ptr = (void*)request;
    event.events = events;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
    if(ret == -1)
        return -1;
}

// 从epoll中删除描述符
int tk_epoll_del(int epoll_fd, int fd, tk_http_request_t* request, int events){
    struct epoll_event event;
    event.data.ptr = (void*)request;
    event.events = events;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
    if(ret == -1)
        return -1;
}

// 返回活跃事件数
int tk_epoll_wait(int epoll_fd, struct epoll_event* events, int max_events, int timeout){
    int ret_count = epoll_wait(epoll_fd, events, max_events, timeout);
    return ret_count;
}

// 分发处理函数
void tk_handle_events(int epoll_fd, int listen_fd, struct epoll_event* events, 
    int events_num, char* path, tk_threadpool_t* tp){

    for(int i = 0; i < events_num; i++){
        // 获取有事件产生的描述符
        tk_http_request_t* request = (tk_http_request_t*)(events[i].data.ptr);
        int fd = request->fd;

        // 有事件发生的描述符为监听描述符
        if(fd == listen_fd) {
            accept_connection(listen_fd, epoll_fd, path);
        }
        else{
            // 有事件发生的描述符为连接描述符

            // 排除错误事件
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
                || (!(events[i].events & EPOLLIN))){
                close(fd);
                continue;
            }

            // 将请求任务加入到线程池中
            int rc = threadpool_add(tp, do_request, events[i].data.ptr);
            // do_request(events[i].data.ptr);
        }
    }
}
