/* 
 * Copyright, TeeKee
 * Date, 2017.7.20
 */

#include<stdlib.h>
#include "epoll.h"
// #include "dbg.h"

struct epoll_event* events;

int tk_epoll_create(int flags){
	int epoll_fd = epoll_create1(flags);
	if(epoll_fd == -1){
		return -1;
	}
	events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    /*
	if(events == NULL){
		return -1;
	}
    */
	return epoll_fd;
}

int tk_epoll_add(int epoll_fd, int fd, struct epoll_event *event){
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, event);
	if(ret == -1){
		return -1;
	}
}

int tk_epoll_mod(int epoll_fd, int fd, struct epoll_event *event){
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, event);
	if(ret == -1){
		return -1;
	}
}

int tk_epoll_del(int epoll_fd, int fd, struct epoll_event *event){
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, event);
	if(ret == -1){
		return -1;
	}
}

int tk_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout){
	int ret_count = epoll_wait(epoll_fd, events, max_events, timeout);
	return ret_count;
}
