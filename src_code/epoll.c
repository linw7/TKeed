//
// Latest edit by TeeKee on 2017/7/23.
//

#include "epoll.h"

struct epoll_event* events;

int tk_epoll_create(int flags){
	int epoll_fd = epoll_create1(flags);
	if(epoll_fd == -1){
		return -1;
	}
	events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * MAXEVENTS);
	return epoll_fd;
}

int tk_epoll_add(int epoll_fd, int fd, tk_http_request_t * request, int events){
	struct epoll_event event;
	event.data.ptr = (void *)request;
	event.events = events;
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
	if(ret == -1){
		return -1;
	}
}

int tk_epoll_mod(int epoll_fd, int fd, tk_http_request_t * request, int events){
	struct epoll_event event;
	event.data.ptr = (void *)request;
	event.events = events;
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
	if(ret == -1){
		return -1;
	}
}

int tk_epoll_del(int epoll_fd, int fd, tk_http_request_t * request, int events){
	struct epoll_event event;
	event.data.ptr = (void *)request;
	event.events = events;
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
	if(ret == -1){
		return -1;
	}
}

int tk_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout){
	int ret_count = epoll_wait(epoll_fd, events, max_events, timeout);
	return ret_count;
}

void tk_handle_events(int epoll_fd, int listen_fd, struct epoll_event* events, 
    int events_num, char* path){
    // events数组由操作系统callback填入等信息
    for(int i = 0; i < events_num; i++){
        // 取出有事件请求的监听符
        tk_http_request_t* request = (tk_http_request_t*)(events[i].data.ptr);
        int fd = request->fd;
        // 监听到请求
        if(fd == listen_fd){
            accept_connection(listen_fd, epoll_fd, path);
        }
        else{
            // 排除错误事件
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))){
                close(fd);
                continue;
            }
            // 处理客户机请求
            do_request(events[i].data.ptr);
        }
    }
}