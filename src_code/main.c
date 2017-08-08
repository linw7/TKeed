//
// Latest edit by TeeKee on 2017/7/23.
//

// 系统头文件
#include <stdio.h>
#include "threadpool.h"
#include "http.h"

#define DEFAULT_CONFIG "tkeed.conf"

extern struct epoll_event *events;
char *conf_file = DEFAULT_CONFIG;
tk_conf_t conf;

int main(int argc, char *argv[]){
    // 读取配置文件
    read_conf(conf_file, &conf);

    // 处理SIGPIPE
    handle_for_sigpipe();

    // 初始化套接字开始监听
    int listen_fd = socket_bind_listen(conf.port);

    // 设置为socket非阻塞
    int rc = make_socket_non_blocking(listen_fd);

    // 创建epoll并注册监听描述符
    int epoll_fd = tk_epoll_create(0);
    tk_http_request_t* request = (tk_http_request_t*)malloc(sizeof(tk_http_request_t));
    tk_init_request_t(request, listen_fd, epoll_fd, conf.root);
    tk_epoll_add(epoll_fd, listen_fd, request, (EPOLLIN | EPOLLET));

    // 初始化线程池
    tk_threadpool_t *tp = threadpool_init(conf.thread_num);

    // 初始化计时器
    tk_timer_init();

    while(1){
        // 得到最近且未删除时间和当前时间差值（等待时间）
        int time = tk_find_timer();

        // 调用epoll_wait函数，返回接收到事件的数量
        int events_num = tk_epoll_wait(epoll_fd, events, MAXEVENTS, -1);

        // 处理已经超时的请求
        tk_handle_expire_timers();

        // 遍历events数组，根据监听种类及描述符类型分发操作
        tk_handle_events(epoll_fd, listen_fd, events, events_num, conf.root, tp);
    }

    // 回收线程资源
    // threadpool_destroy(tp, graceful_shutdown);
}

