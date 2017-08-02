
# TKeed WebServer

[![Build Status](https://travis-ci.org/linw7/TKeed.svg?branch=master)](https://travis-ci.org/linw7/TKeed)
[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)

---

## 时间轴

~~于Linux环境下开发，前期开发使用Sublime，但无法完整调试，只能逐个模块编译测试是否有语法错误、链接错误。功能测试只有工具模块priority_queue和list可以单独用Mock数据跑通，util模块可提前和主模块链接并调试，其他模块需要或多或少有依赖关系。后期工程文件较多之后改用Clion开发 + 单步调试。~~

- v1.0已经完成，本地已调试通过。提交到GitHub上的代码会由Travis自动构建。

## 开发环境

开发工具：

- 操作系统：Ubuntu 16.04

- 编辑器：Sublime + Vim

- 编译器：gcc 5.4.0

- 单元测试：~~gtest~~

- 版本控制：git

- 代码结构：[Understand](https://scitools.com/) + [callgraph](http://blog.csdn.net/solstice/article/details/488865)

- 集成环境：[Clion](https://www.jetbrains.com/clion/)

新增工具：

- 自动化构建：[Travis CI](https://travis-ci.org/linw7/TKeed)

- 压测工具：[WebBench](https://github.com/EZLippi/WebBench)

---

## 具体实现

### 并发模型

并发模型主要有多进程模型、多线程模型和事件驱动模型（select, poll, epoll）。为了减少创建进程、线程创建的开销，在并发服务器中也常设置进程池和线程池，这样在有新连接到来时就不需要重新创建造成不必要的开销。除此之外，使用epoll时，任务被拆分成了独立事件，各个事件可以单独执行，所以也可以将二者结合，TKeed正是如此。

- [epoll (kernel 2.6+)](https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/)

- non-blocking I/O

- threadpool

Linux内核2.6之后开始支持epoll，也是本服务器的核心。epoll模型中内核相当于监控代理，监控的粒度为每一个事件，我们把每个完整的处理过程分拆成了多个独立的事件并在epoll中注册，之后监控是否有事件发生的任务就交给内核来做，一旦监测到事件就分发到相应处理模块。就HTTP服务器而言，可以分为以下几步。在创建好了epoll之后：

- 首先需要注册到"监听事件"，之后不需要一直等待下去，直接返回（异步非阻塞）。

- 一旦内核监听到请求就会自动通知可以去建立连接并创建连接描述符，该连接描述符被注册到"读事件"，之后立即返回。

- 用户发送的数据到达服务器，内核感知到读事件，通知服务器来读取数据，服务器读取请求数据后开始解析并返回。

- 文件I/O完成，内核通知写有写事件到来，之后数据调用write函数发送至客户端。

当然，TKeed并不仅仅只用到了epoll和异步非阻塞I/O模型。监听事件最先被注册，在此之后不再阻塞监听，当内核监听到事件发生之后，立即建立连接并注册读事件，读事件并不会阻塞等待用户数据，一直等到内核通知该事件到来才去获取请求，这部分使用的是epoll + 异步非阻塞I/O模型。当请求到达之后，处理请求的操作被放到线程池中，等待多个线程并发响应处理，即使某一线程读取本地文件时被阻塞也会有其他线程可以被调度执行。

之所以选择epoll模型是因为事件驱动适合I/O密集型操作，而HTTP服务器最核心的任务就是响应请求的数据，涉及大量I/O请求。另外当并发量上来之后，传统的多进程、多线程模型虽然并发量很大，但大多处于阻塞状态，即使多为就绪态，系统调度开销也非常大，因此这里使用事件驱动模型无疑更适合。

### 核心结构体

1. 配置信息结构(unil.h)
```C++
typedef struct tk_conf{
    char root[PATHLEN];    // 文件根目录
    int port;    // 端口号
    int thread_num;    // 线程数（线程池大小）
}tk_conf_t;
```

2. 请求信息结构(http_request.h)
```C++
typedef struct tk_http_request{
    char* root;    // 配置目录
    int fd;    // 描述符（监听、连接）
    int epoll_fd;    // epoll描述符
    char buff[MAX_BUF];    // 用户缓冲
    int method;    // 请求方法
    int state;    // 请求头解析状态
    // 以下主要为标记解析请求时索引信息
    // 部分未使用，用于扩展功能
    size_t pos;
    size_t last;
    void *request_start;
    void *method_end;
    void *uri_start;
    void *uri_end;
    void *path_start;
    void *path_end;
    void *query_start;
    void *query_end;
    int http_major;
    int http_minor;
    void *request_end;
    struct list_head list;
    void *cur_header_key_start;
    void *cur_header_key_end;
    void *cur_header_value_start;
    void *cur_header_value_end;
    void *timer;    // 指向时间戳结构
}tk_http_request_t;
```

3. 响应头结构(http_requesh.h)
```C++
typedef struct tk_http_out{
    int fd;    // 连接描述符
    int keep_alive;    // HTTP连接状态
    time_t mtime;    // 文件类型
    int modified;    // 是否修改
    int status;    // 返回码
}tk_http_out_t;
```

4. 优先队列结构(priority_queue.h)
```C++
typedef struct priority_queue{
    void **pq;    // 优先队列节点指针
    size_t nalloc;    // 优先队列实际元素个数
    size_t size;    // 优先队列大小
    tk_pq_comparator_pt comp;    // 堆模式
}tk_pq_t;
```

5. 时间结构(timer.h)
```C++
typedef struct tk_timer{
    size_t key;    // 标记超时时间
    int deleted;    // 标记是否被删除
    timer_handler_pt handler;    // 超时处理
    tk_http_request_t *request;    // 指向对应的request请求
} tk_timer_t;
```

6. I/O包结构(rio.h)
```C++
typedef struct{
    int rio_fd;    // 描述符
    ssize_t rio_cnt;    // buf中未读字节数
    char *rio_bufptr;    // 下一个未读字符指针
    char rio_buf[RIO_BUFSIZE];    // 缓冲
}rio_t;
```

7. 线程池结构（threadpool.h）
```C++
typedef struct threadpool{
	pthread_mutex_t lock;    // 互斥锁
	pthread_cond_t cond;    // 条件变量
	pthread_t *threads;    // 线程
	tk_task_t *head;    // 任务链表
	int thread_count;     // 线程数
	int queue_size;    // 任务链表长
	int shutdown;    // 关机方式
	int started;
}tk_threadpool_t;
```

8. 任务结构（threadpool.h）
```C++
typedef struct tk_task{
	void (*func)(void *);    //
	void *arg;    //
	struct tk_task *next;    // 任务链表（下一节点指针）
}tk_task_t;
```

### 调用树

![调用树](./datum/ClusterCall_full.jpg)

### 主要函数

1. util.c

    - 读配置：int read_conf(char* filename, tk_conf_t* conf);

    - 绑定监听：int socket_bind_listen(int port);

    - 处理连接：void accept_connection(int listen_fd, int epoll_fd, char* path);

2. epoll.c

    - 创建epoll：int tk_epoll_create(int flags);

    - 添加到epoll：int tk_epoll_add(int epoll_fd, int fd, tk_http_request_t* request, int events);

    - 从epoll删除：int tk_epoll_del(int epoll_fd, int fd, tk_http_request_t* request, int events);

    - 修改事件状态：int tk_epoll_mod(int epoll_fd, int fd, tk_http_request_t* request, int events);

    - 等待事件：int tk_epoll_wait(int epoll_fd, struct epoll_event* events, int max_events, int timeout);

    - 分发对应事件：void tk_handle_events(int epoll_fd, int listen_fd, struct epoll_event* events, int events_num, char* path);

- http.c

    - 处理请求总入口：void do_request(void* ptr);

    - 解析URI：void parse_uri(char* uri, int length, char* filename, char *query);

    - 获取文件类型：const char* get_file_type(const char* type);

    - 错误信息处理：void do_error(int fd, char* cause, char* err_num, char* short_msg, char* long_msg);

    - 响应静态文件：void serve_static(int fd, char* filename, size_t filesize, tk_http_out_t* out);

- http_parse.c

    - 解析请求行：int tk_http_parse_request_line(tk_http_request_t* request);

    - 解析请求体：int tk_http_parse_request_body(tk_http_request_t* request);

- http_request.c

    - 初始化请求头结构：int tk_init_request_t(tk_http_request_t* request, int fd, int epoll_fd, char* path);

    - 删除请求头结构：int tk_free_out_t(tk_http_out_t* out);

    - 初始化响应结构：int tk_init_out_t(tk_http_out_t* out, int fd);

    - 删除响应头结构：int tk_free_out_t(tk_http_out_t* out);

    - 获取状态码对应提示：const char* get_shortmsg_from_status_code(int status_code);

    - 关闭连接：int tk_http_close_conn(tk_http_request_t* request);

- timer.c

    - 刷新当前时间：void tk_time_update();
    
    - 初始化时间：int tk_timer_init();

    - 新增时间戳：void tk_add_timer(tk_http_request_t* request, size_t timeout, timer_handler_pt handler);

    - 删除时间戳：void tk_del_timer(tk_http_request_t* request);

    - 处理超时：void tk_handle_expire_timers();

- threadpool.c

    - 初始化线程池：tk_threadpool_t* threadpool_init(int thread_num);

    - 添加任务：threadpool_add(tk_threadpool_t* pool, void (* func)(void*), void* arg);

    - 释放线程池及任务：threadpool_free(tk_threadpool_t* pool);

    - 回收线程资源：int threadpool_destory(tk_threadpool_t* pool, int graceful);

    - 工作线程：void* threadpool_worker(void* arg);

### 特性

- 使用状态机解析HTTP协议，非简单字符串匹配方式解析请求

- 添加Timer定时器，定时回调tk_http_close_conn处理超时请求

    - 高效的小根堆结构

    - 惰性删除方式

- 实现了HTTP持续连接传输数据

- 线程池操作及其同步互斥管理


---

## 背景知识

### 网络基础

**应用层**

HTTP协议工作在应用层，端口号是80。HTTP协议被用于网络中两台计算机间的通信，相比于TCP/IP这些底层协议，HTTP协议更像是高层标记型语言，浏览器根据从服务器得到的HTTP响应体中分别得到报文头，响应头和信息体（HTML正文等），之后将HTML文件解析并呈现在浏览器上。同样，我们在浏览器地址栏输入网址之后，浏览器相当于用户代理帮助我们组织好报文头，请求头和信息体（可选），之后通过网络发送到服务器，服务器根据请求的内容准备数据。所以如果想要完全弄明白HTTP协议，你需要写一个浏览器 + 一个Web服务器，一侧来生成请求信息，一侧生成响应信息。

从网络分层模型来看，HTTP工作在应用层，其在传输层由TCP协议为其提供服务。所以可以猜到，HTTP请求前，客户机和服务器之间一定已经通过三次握手建立起连接，其中套接字中服务器一侧的端口号为HTTP周知端口80。在请求和传输数据时也是有讲究的，通常一个页面上不只有文本数据，有时会内嵌很多图片，这时候有两种选择可以考虑。一种是对每一个文件都建立一个TCP连接，传送完数据后立马断开，通过多次这样的操作获取引用的所有数据，但是这样一个页面的打开需要建立多次连接，效率会低很多。另一种是对于有多个资源的页面，传送完一个数据后不立即断开连接，在同一次连接下多次传输数据直至传完，但这种情况有可能会长时间占用服务器资源，降低吞吐率。上述两种模式分别是HTTP 1.0和HTTP 1.1版本的默认方式，具体是什么含义会在后面详细解释。

- HTTP工作流程

    一次完整的HTTP请求事务包含以下四个环节：

    - 建立起客户机和服务器连接。

    - 建立连接后，客户机发送一个请求给服务器。

    - 服务器收到请求给予响应信息。

    - 客户端浏览器将返回的内容解析并呈现，断开连接。

- HTTP协议结构

    **请求报文**

    对于HTTP请求报文我们可以通过以下两种方式比较直观的看到：一是在浏览器调试模式下（F12）看请求响应信息，二是通过wireshark或者tcpdump抓包实现。通过前者看到的数据更加清晰直观，通过后者抓到的数据更真实。但无论是用哪种方式查看，得到的请求报文主题体信息都是相同的，对于请求报文，主要包含以下四个部分，每一行数据必须通过"\r\n"分割，这里可以理解为行末标识符。

    - 报文头（只有一行）

        结构：method  uri  version

        - method

            HTTP的请求方法，一共有9中，但GET和POST占了99%以上的使用频次。GET表示向特定资源发起请求，当然也能提交部分数据，不过提交的数据以明文方式出现在URL中。POST通常用于向指定资源提交数据进行处理，提交的数据被包含在请求体中，相对而言比较安全些。

        - uri

            用来指代请求的文件，≠URL。

        - version

            HTTP协议的版本，该字段有HTTP/1.0和HTTP/1.1两种。

    - 请求头（多行）

        在HTTP/1.1中，请求头除了Host都是可选的。包含的头五花八门，这里只介绍部分。

        - Host：指定请求资源的主机和端口号。端口号默认80。

        - Connection：值为keep-alive和close。keep-alive使客户端到服务器的连接持续有效，不需要每次重连，此功能为HTTP/1.1预设功能。

        - Accept：浏览器可接收的MIME类型。假设为text/html表示接收服务器回发的数据类型为text/html，如果服务器无法返回这种类型，返回406错误。

        - Cache-control：缓存控制，Public内容可以被任何缓存所缓存，Private内容只能被缓存到私有缓存，non-cache指所有内容都不会被缓存。

        - Cookie：将存储在本地的Cookie值发送给服务器，实现无状态的HTTP协议的会话跟踪。

        - Content-Length：请求消息正文长度。

        另有User-Agent、Accept-Encoding、Accept-Language、Accept-Charset、Content-Type等请求头这里不一一罗列。由此可见，请求报文是告知服务器请求的内容，而请求头是为了提供服务器一些关于客户机浏览器的基本信息，包括编码、是否缓存等。

    - 空行（一行）

    - 可选消息体（多行）

    **响应报文**

    响应报文是服务器对请求资源的响应，通过上面提到的方式同样可以看到，同样地，数据也是以"\r\n"来分割。

    - 报文头（一行）

        结构：version status_code status_message

        - version

            描述所遵循的HTTP版本。

        - status_code

            状态码，指明对请求处理的状态，常见的如下。

            - 200：成功。

            - 301：内容已经移动。

            - 400：请求不能被服务器理解。

            - 403：无权访问该文件。

            - 404：不能找到请求文件。

            - 500：服务器内部错误。

            - 501：服务器不支持请求的方法。

            - 505：服务器不支持请求的版本。

        - status_message

            显示和状态码等价英文描述。

    - 响应头（多行）

        这里只罗列部分。

        - Date：表示信息发送的时间。

        - Server：Web服务器用来处理请求的软件信息。

        - Content-Encoding：Web服务器表明了自己用什么压缩方法压缩对象。

        - Content-Length：服务器告知浏览器自己响应的对象长度。

        - Content-Type：告知浏览器响应对象类型。

    - 空行（一行）

    - 信息体（多行）

        实际有效数据，通常是HTML格式的文件，该文件被浏览器获取到之后解析呈现在浏览器中。

    **CGI与环境变量**

    - CGI程序

        服务器为客户端提供动态服务首先需要解决的是得到用户提供的参数再根据参数信息返回。为了和客户端进行交互，服务器需要先创建子进程，之后子进程执行相应的程序去为客户服务。CGI正是帮助我们解决参数获取、输出结果的。

        动态内容获取其实请求报文的头部和请求静态数据时完全相同，但请求的资源从静态的HTML文件变成了后台程序。服务器收到请求后fork()一个子进程，子进程执行请求的程序，这样的程序称为CGI程序（Python、Perl、C++等均可）。通常在服务器中我们会预留一个单独的目录（cgi-bin）用来存放所有的CGI程序，请求报文头部中请求资源的前缀都是/cgi-bin，之后加上所请求调用的CGI程序即可。

        所以上述流程就是：客户端请求程序 -> 服务器fork()子进程 -> 执行被请求程序。接下来需要解决的问题就是如何获取客户端发送过来的参数和输出信息怎么传递回客户端。

    - 环境变量

        对CGI程序来说，CGI环境变量在创建时被初始化，结束时被销毁。当CGI程序被HTTP服务器调用时，因为是被服务器fork()出来的子进程，所以其继承了其父进程的环境变量，这些环境变量包含了很多基本信息，请求头中和响应头中列出的内容（比如用户Cookie、客户机主机名、客户机IP地址、浏览器信息等），CGI程序所需要的参数也在其中。

    - GET方法下参数获取

        服务器把接收到的参数数据编码到环境变量QUERY_STRING中，在请求时只需要直接把参数写到URL最后即可，比如"http:127.0.0.1:80/cgi-bin/test?a=1&b=2&c=3"，表示请求cgi-bin目录下test程序，'?'之后部分为参数，多个参数用'&'分割开。服务器接收到请求后环境变量QUERY_STRING的值即为a=1&b=2&c=3。

        在CGI程序中获取环境变量值的方法是：getenv()，比如我们需要得到上述QUERY_STRING的值，只需要下面这行语句就可以了。

            char *value = getenv("QUERY_STRING");

        之后对获得的字符串处理一下提取出每个参数信息即可。

    - POST方法下参数获取

        POST方法下，CGI可以直接从服务器标准输入获取数据，不过要先从CONTENT_LENGTH这个环境变量中得到POST参数长度，再获取对应长度内容。

    **会话机制**

    HTTP作为无状态协议，必然需要在某种方式保持连接状态。这里简要介绍一下Cookie和Session。

    - Cookie

        Cookie是客户端保持状态的方法。

        Cookie简单的理解就是存储由服务器发至客户端并由客户端保存的一段字符串。为了保持会话，服务器可以在响应客户端请求时将Cookie字符串放在Set-Cookie下，客户机收到Cookie之后保存这段字符串，之后再请求时候带上Cookie就可以被识别。

        除了上面提到的这些，Cookie在客户端的保存形式可以有两种，一种是会话Cookie一种是持久Cookie，会话Cookie就是将服务器返回的Cookie字符串保持在内存中，关闭浏览器之后自动销毁，持久Cookie则是存储在客户端磁盘上，其有效时间在服务器响应头中被指定，在有效期内，客户端再次请求服务器时都可以直接从本地取出。需要说明的是，存储在磁盘中的Cookie是可以被多个浏览器代理所共享的。

    - Session

        Session是服务器保持状态的方法。

        首先需要明确的是，Session保存在服务器上，可以保存在数据库、文件或内存中，每个用户有独立的Session用户在客户端上记录用户的操作。我们可以理解为每个用户有一个独一无二的Session ID作为Session文件的Hash键，通过这个值可以锁定具体的Session结构的数据，这个Session结构中存储了用户操作行为。

    当服务器需要识别客户端时就需要结合Cookie了。每次HTTP请求的时候，客户端都会发送相应的Cookie信息到服务端。实际上大多数的应用都是用Cookie来实现Session跟踪的，第一次创建Session的时候，服务端会在HTTP协议中告诉客户端，需要在Cookie里面记录一个Session ID，以后每次请求把这个会话ID发送到服务器，我就知道你是谁了。如果客户端的浏览器禁用了Cookie，会使用一种叫做URL重写的技术来进行会话跟踪，即每次HTTP交互，URL后面都会被附加上一个诸如sid=xxxxx这样的参数，服务端据此来识别用户，这样就可以帮用户完成诸如用户名等信息自动填入的操作了。

**传输层**

传输层主要需要了解TCP建立连接过程和客户机-服务器状态变化。深入了解传输层的话，抓包（Wireshark或Tcpdump）无疑是最好的。[详见笔记](https://github.com/linw7/Skill-Tree/blob/master/%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%BD%91%E7%BB%9C.md)。

**网络层**

网络层部分对于服务器而言过于底层，这里不再介绍，[详见笔记](https://github.com/linw7/Skill-Tree/blob/master/%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%BD%91%E7%BB%9C.md)。

### 客户端 - 服务器模型


**客户端**

- 创建socket -> int socket(int domain, int type, int protocol);
    
- 连接指定计算机 -> int connect(int sockfd, struct sockaddr* addr, socklen_t addrlen);
                
    - sockfd客户端的sock描述字。
    
    - addr：服务器的地址。
    
    - addrlen：socket地址长度。
    
- 向socket写入信息 -> ssize_t write(int fd, const void *buf, size_t count);
        
    - fd、buf、count：同read中意义。
    
    - 大于0表示写了部分或全部数据，小于0表示出错。
    
- 关闭oscket -> int close(int fd);
                - fd：同服务器端fd。

**服务器端**

- 创建socket -> int socket(int domain, int type, int protocol);
        
    - domain：协议域，决定了socket的地址类型，IPv4为AF_INET。
      
    - type：指定socket类型，SOCK_STREAM为TCP连接。
      
    - protocol：指定协议。IPPROTO_TCP表示TCP协议，为0时自动选择type默认协议。
      
- 绑定socket和端口号 -> int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
                
    - sockfd：socket返回的套接字描述符，类似于文件描述符fd。
      
    - addr：有个sockaddr类型数据的指针，指向的是被绑定结构变量。
                
    ```C++
    // IPv4的sockaddr地址结构
    struct sockaddr_in {
        sa_family_t sin_family;    // 协议类型，AF_INET
        in_port_t sin_port;    // 端口号
        struct in_addr sin_addr;    // IP地址
    };
    struct in_addr {
        uint32_t s_addr;
    }
    ```

    - addrlen：地址长度。
      
- 监听端口号 -> int listen(int sockfd, int backlog);
             
    - sockfd：要监听的sock描述字。
      
    - backlog：socket可以排队的最大连接数。
      
- 接收用户请求 -> int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
     
    - sockfd：服务器socket描述字。
      
    - addr：指向地址结构指针。
      
    - addrlen：协议地址长度。
      
    - 注：一旦accept某个客户机请求成功将返回一个全新的描述符用于标识具体客户的TCP连接。
      
- 从socket中读取字符 -> ssize_t read(int fd, void *buf, size_t count);
                
    - fd：连接描述字。
      
    - buf：缓冲区buf。
      
    - count：缓冲区长度。
      
    - 注：大于0表示读取的字节数，返回0表示文件读取结束，小于0表示发生错误。
        
- 关闭socket -> int close(int fd);
        
    - fd：accept返回的连接描述字，每个连接有一个，生命周期为连接周期。
        
    - 注：sockfd是监听描述字，一个服务器只有一个，用于监听是否有连接；fd是连接描述字，用于每个连接的操作。

### 实现方案

基本的多线程多进程方案和优劣参考[操作系统专题](https://github.com/linw7/Skill-Tree/blob/master/%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F.md)。下面主要讲一下网络I/O模型。

在描述网络I/O模型的诸多书籍中，很多都只说笼统的概念，我们将问题具体化，暂时只考虑服务器端的网络I/O情形。我们假定目前的情形是服务器已经在监听用户请求，建立连接后服务器调用read()函数等待读取用户发送过来的数据流，之后将接收到的数据打印出来。

所以服务器端简单是这样的流程：建立连接 -> 监听请求 -> 等待用户数据 -> 打印数据。我们总结网络通信中的等待：

- 建立连接时等待对方的ACK包。

- 等待客户端请求。

- 输入等待：服务器用户数据到达内核缓冲区（read函数等待）。

- 输出等待：用户端等待缓冲区有足够空间可以输入（write函数等待）。

另外为了能够解释清楚网络I/O模型，还需要了解一些基础。对服务器而言，打印出用户输入的字符串（printf函数）和从网络中获取数据（read函数）需要单独来看。服务器首先accept用户连接请求后首先调用read函数等待数据，这里的read函数是系统调用，运行于内核态，使用的也是内核地址空间，并且从网络中取得的数据需要先写入到内核缓冲区。当read系统调用获取到数据后将这些数据再复制到用户地址空间的用户缓冲区中，之后返回到用户态执行printf函数打印字符串。我们需要明确两点：

- read执行在内核态且数据流先读入内核缓冲区；printf运行于用户态，打印的数据会先从内核缓冲区复制到进程的用户缓冲区，之后打印出来。

- printf函数一定是在read函数已经准备好数据之后才能执行，但read函数作为I/O操作通常需要等待而触发阻塞。调用read函数的是服务器进程，一旦被read调用阻塞，整个服务器在获取到用户数据前都不能接受任何其他用户的请求（单进程/线程）。

有了上面的基础，我们就可以介绍下面四种网路I/O模型。

**阻塞式**

- 阻塞表示一旦调用I/O函数必须等整个I/O完成才返回。正如上面提到的那种情形，当服务器调用了read函数之后，如果不是立即接收到数据，服务器进程会被阻塞，之后一直在等待用户数据到达，用户数据到达后首先会写进内核缓冲区，之后内核缓冲区数据复制到用户进程（服务器进程）缓冲区。完成了上述所有的工作后，才会把执行权限返回给用户（从内核态 -> 用户态）。

- 很显然，阻塞式I/O的效率实在太低，如果用户输入数据迟迟不到的话，整个服务器就会一直被阻塞（单进程/线程）。为了不影响服务器接收其他进程的连接，我们可以考虑多进程模型，这样当服务器建立连接后为连接的用户创建新线程，新线程即使是使用阻塞式I/O也仅仅是这一个线程被阻塞，不会影响服务器等待接收新的连接。

- 多线程模型下，主线程等待用户请求，用户有请求到达时创建新线程。新线程负责具体的工作，即使是因为调用了read函数被阻塞也不会影响服务器。我们还可以进一步优化创建连接池和线程池以减小频繁调用I/O接口的开销。但新问题随之产生，每个新线程或者进程（加入使用对进程模型）都会占用大量系统资源，除此之外过多的线程和进程在调度方面开销也会大很对，所以这种模型并不适合大并发量。

**非阻塞I/O**

- 阻塞和非阻塞最大的区别在于调用I/O系统调用后，是等整个I/O过程完成再把操作权限返回给用户还是会立即返回。

- 可以使用以下语句将句柄fd设置为非阻塞I/O：fcntl(fd, F_SETFL, O_NONBLOCK);

- 非阻塞I/O在调用后会立即返回，用户进程对返回的返回值判断以区分是否完成了I/O。如果返回大于0表示完成了数据读取，返回值即读取的字节数；返回0表示连接已经正常断开；返回-1表示错误，接下来用户进程会不停地询问kernel是否准备完毕。

- 非阻塞I/O虽然不再会完全阻塞用户进程，但实际上由于用户进程需要不停地询问kernel是否准备完数据，所以整体效率依旧非常低，不适合做并发。

**I/O多路复用（事件驱动模型）**

前面已经论述了多进程、多进程模型会因为开销巨大和调度困难而导致并不能承受高并发量。但不适用这种模型的话，无论是阻塞还是非阻塞方式都会导致整个服务器停滞。

所以对于大并发量，我们需要一种代理模型可以帮助我们集中去管理所有的socket连接，一旦某个socket数据到达了就执行其对应的用户进程，I/O多路复用就是这么一种模型。Linux下I/O多路复用的系统调用有select，poll和epoll，但从本质上来讲他们都是同步I/O范畴。

1. select

    - 相关接口：

        int select (int maxfd, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout);
        
        FD_ZERO(int fd, fd_set* fds)    //清空集合
        
        FD_SET(int fd, fd_set* fds)    //将给定的描述符加入集合
        
        FD_ISSET(int fd, fd_set* fds)    //将给定的描述符从文件中删除
        
        FD_CLR(int fd, fd_set* fds)    //判断指定描述符是否在集合中

    - 参数：
    
        maxfd：当前最大文件描述符的值+1（≠ MAX_CONN）。
        
        readfds：指向读文件队列集合（fd_set）的指针。
        
        writefds：同上，指向读集合的指针。
        
        writefds：同上，指向错误集合的指针。
        
        timeout：指向timeval结构指针，用于设置超时。

    - 其他：

        判断和操作对象为set_fd集合，集合大小为单个进程可打开的最大文件数1024或2048（可重新编译内核修改但不建议）。

2. poll
    
    - 相关接口：
    
        int poll(struct pollfd *fds, unsigned int nfds, int timeout);

    - 结构体定义：
        ```C++
        struct pollfd{
            int fd;    // 文件描述符
            short events;    // 等到的事件
            short revents;    // 实际发生的事件
        }
        ```

    - 参数：
            
        fds：指向pollfd结构体数组的指针。
        
        nfds：pollfd数组当前已被使用的最大下标。
        
        timeout：等待毫秒数。

    - 其他：
    
        判断和操作对象是元素为pollfd类型的数组，数组大小自己设定，即为最大连接数。

3. epoll
    
    - 相关接口：
            
        int epoll_create(int size);    // 创建epoll句柄
        
        int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);    // 事件注册函数
        
        int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);

    - 结构体定义：
        ```C++
        struct epoll_event{
            __uint32_t events;
            epoll_data_t data;
        };
        typedef union epoll_data{
            void *ptr;
            int fd;
            __uint32_t u32;
            __uint64_t u64;
        }epoll_data_t;
        ```

    - 参数：
            
        size：用来告诉内核要监听的数目。
                
        epfd：epoll函数的返回值。
                
        op：表示动作（EPOLL_CTL_ADD/EPOLL_CTL_FD/EPOLL_CTL_DEL）。
                
        fd：需要监听的fd。
                
        events：指向epoll_event的指针，该结构记录监听的事件。
                
        maxevents：告诉内核events的大小。
                
        timeout：超时时间（ms为单位，0表示立即返回，-1将不确定）。

4. select、poll和epoll区别
        
    - 操作方式及效率：
        
        select是遍历，需要遍历fd_set每一个比特位（= MAX_CONN），O(n)；poll是遍历，但只遍历到pollfd数组当前已使用的最大下标（≠ MAX_CONN），O(n)；epoll是回调，O(1)。
    
    - 最大连接数：
        
        select为1024/2048（一个进程打开的文件数是有限制的）；poll无上限；epoll无上限。
    
    - fd拷贝：
            
        select每次都需要把fd集合从用户态拷贝到内核态；poll每次都需要把fd集合从用户态拷贝到内核态；epoll调用epoll_ctl时拷贝进内核并放到事件表中，但用户进程和内核通过mmap映射共享同一块存储，避免了fd从内核赋值到用户空间。
        
    - 其他：
        
        select每次内核仅仅是通知有消息到了需要处理，具体是哪一个需要遍历所有的描述符才能找到。epoll不仅通知有I/O到来还可通过callback函数具体定位到活跃的socket，实现伪AIO。

**异步I/O模型**

- 上面三种I/O方式均属于同步I/O。

- 从阻塞式I/O到非阻塞I/O，我们已经做到了调用I/O请求后立即返回，但不停轮询的操作效率又很低，如果能够既像非阻塞I/O能够立即返回又能不一直轮询的话会更符合我们的预期。

- 之所以用户进程会不停轮询就是因为在数据准备完毕后内核不会回调用户进程，只能通过用户进程一次又一次轮询来查询I/O结果。如果内核能够在完成I/O后通过消息告知用户进程来处理已经得到的数据自然是最好的，异步I/O就是这么回事。

- 异步I/O就是当用户进程发起I/O请求后立即返回，直到内核发送一个信号，告知进程I/O已完成，在整个过程中，都没有进程被阻塞。看上去异步I/O和非阻塞I/O的区别在于：判断数据是否准备完毕的任务从用户进程本身被委托给内核来完成。这里所谓的异步只是操作系统提供的一直机制罢了。

---

## 项目目的

- 软件开发流程

    遵循完整开发流程，确定需求 -> 选定服务器模型 -> 定义数据结构 -> 开发辅助工具 -> 单元测试 -> 核心部分开发 -> 集成测试 -> 性能测试。开发环境也统一到Linux环境下，通过git进行版本控制，尽可能模拟真实工作环境。

- 基础知识

    开发HTTP服务器从宏观上来说会对网络协议TCP及其各个状态理解更深，会对HTTP协议主要字段的功能理解更深，会对操作系统中多线程、多进程并发概念和局限性理解更深刻，会对网络I/O模型认识更深。

- 数据结构

    通过对场景需求和将来扩展性的研究，需要设计合理的、高效的数据结构，比如在本项目中最核心的tk_request_t、tk_timer_t和tk_pq_t结构考虑到了扩展性和操作高效性（比如tk_time_t中的deleted字段、tk_pq_t中的size字段）。同时根据需求实现了list和priority_queue库并提供统一接口，可以帮助更好地掌握数据结构和设计接口。

- 编程语言

    项目中涉及C语言中方方面面，比如预定义、typedef、全局变量、静态全局变量、函数指针、位运算、强制类型转换、结构体操作等，很多在调试时候遇到过问题，但也能学到很多。另外，也顺带学些了很多编译和调试的小技巧。

- 开发工具

    最后，在开发过程中使用到的都是最基本、最常用的开发工具，开发、调试、版本控制都有所涉及，可以更好地利用辅助工具完成开发任务。

---
