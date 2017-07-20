/* 
 * Copyright, TeeKee
 * Date, 2017.7.20
 */

// 系统头文件
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
// 封装并实现新功能
#include "epoll.h"
// #include "threadpool.h"
// 本地功能
// #include "timer.h"
// #include "http.h"
#include "util.h"

#define DEFAULT_CONFIG "tkeed.conf"
#define VERSION "0.1"

extern struct epoll_event *events;

int main(int argc, char *argv[]){
    // tk_conf_t
    tk_conf_t conf;
	char *conf_file = DEFAULT_CONFIG;
	char conf_buff[BUFLEN];
	int read_conf_status = read_conf(conf_file, &conf, conf_buff, BUFLEN);
    printf("Read config ok !\n");
    printf("The port = %d, the thread number = %d ! \n\n", conf.port, conf.thread_num);
    /*
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	if(sigaction(SIGPIPE, &sa, NULL)){
		log_err("install signal handler for SIGPIPE failed");
		return 0;
	}
    */
    // 初始化套接字开始监听
	int listen_fd = socket_bind_listen(conf.port);
    if(listen_fd != -1){
        printf("Server is listening !\n\n");
    }
    // 设置为socket非阻塞
	int rc = make_socket_non_blocking(listen_fd);
    if(rc != -1){
        printf("Make socket non_blocking success !\n\n");
    }
    // 创建epoll并注册监听描述符
    int epoll_fd = tk_epoll_create(0);
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = (void *)NULL;
    tk_epoll_add(epoll_fd, listen_fd, &event);

    printf("Epoll init ok ! \n\n");

    /*
    while(1){

    }
    */
}
