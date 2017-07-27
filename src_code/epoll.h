//
// Latest edit by TeeKee on 2017/7/23.
//

#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include "http.h"
#include "threadpool.h"

#define MAXEVENTS 1024

int tk_epoll_create(int flags);
int tk_epoll_add(int epoll_fd, int fd, tk_http_request_t* request, int events);
int tk_epoll_mod(int epoll_fd, int fd, tk_http_request_t* request, int events);
int tk_epoll_del(int epoll_fd, int fd, tk_http_request_t* request, int events);
int tk_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout);
void tk_handle_events(int epoll_fd, int listen_fd, struct epoll_event* events,
                      int events_num, char* path, tk_threadpool_t* tp);

#endif
