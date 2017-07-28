//
// Latest edit by TeeKee on 2017/7/23.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "util.h"
#include "http_request.h"
#include "epoll.h"

int read_conf(char* filename, tk_conf_t* conf){
    // 以只读方式打开文件
    FILE* fp = fopen(filename, "r");
    if(!fp)
        return TK_CONF_ERROR;

    char buff[BUFLEN];
    int buff_len = BUFLEN;
    char* curr_pos = buff;
    char* delim_pos = NULL;
    int i = 0;
    int pos = 0;
    int line_len = 0;
    while(fgets(curr_pos, buff_len - pos, fp)){
        // 定位每行第一个界定符位置
        delim_pos = strstr(curr_pos, DELIM);
        if(!delim_pos)
            return TK_CONF_ERROR;
        if(curr_pos[strlen(curr_pos) - 1] == '\n'){
            curr_pos[strlen(curr_pos) - 1] = '\0';
        }

        // 得到root信息
        if(strncmp("root", curr_pos, 4) == 0){
            delim_pos = delim_pos + 1;
            while(*delim_pos != '#'){
                conf->root[i++] = *delim_pos;
                ++delim_pos;
            }
        }

        // 得到port值
        if(strncmp("port", curr_pos, 4) == 0)
            conf->port = atoi(delim_pos + 1);

        // 得到thread数量
        if(strncmp("thread_num", curr_pos, 9) == 0)
            conf->thread_num = atoi(delim_pos + 1);

        // line_len得到当前行行长
        line_len = strlen(curr_pos);

        // 当前位置跳转至下一行首部
        curr_pos += line_len;
    }
    fclose(fp);
    return TK_CONF_OK;
}

void handle_for_sigpipe(){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))
        return;
}

int socket_bind_listen(int port){
    // 检查port值，取正确区间范围
    port = ((port <= 1024) || (port >= 65535)) ? 6666 : port;

    // 创建socket(IPv4 + TCP)，返回监听描述符
    int listen_fd = 0;
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    // 消除bind时"Address already in use"错误
    int optval = 1;
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) == -1){
        return -1;
    }

    // 设置服务器IP和Port，和监听描述副绑定
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        return -1;

    // 开始监听，最大等待队列长为LISTENQ
    if(listen(listen_fd, LISTENQ) == -1)
        return -1;

    // 无效监听描述符
    if(listen_fd == -1){
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

int make_socket_non_blocking(int fd){
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) == -1)
        return -1;
}

void accept_connection(int listen_fd, int epoll_fd, char* path){
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = 0;
    int accept_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if(accept_fd == -1)
        perror("accept");

    // 设为非阻塞模式
    int rc = make_socket_non_blocking(accept_fd);

    // 申请tk_http_request_t类型节点并初始化
    tk_http_request_t* request = (tk_http_request_t*)malloc(sizeof(tk_http_request_t));
    tk_init_request_t(request, accept_fd, epoll_fd, path);

    // 文件描述符可以读，边缘触发(Edge Triggered)模式，保证一个socket连接在任一时刻只被一个线程处理
    tk_epoll_add(epoll_fd, accept_fd, request, (EPOLLIN | EPOLLET | EPOLLONESHOT));

    // 新增时间信息
    tk_add_timer(request, TIMEOUT_DEFAULT, tk_http_close_conn);
}

