# TKeed启示录

1. [C10K问题](https://www.oschina.net/translate/c10k)本质？

    - C10K问题本质是操作系统问题，创建线程多了，数据频繁拷贝（I/O，内核数据拷贝到用户进程空间、阻塞），进程/线程上下文切换消耗大，从而导致操作系统崩溃。

2. 为什么使用长连接，怎么实现的？

    - 长连接指在一个TCP连接上可以发送多个HTTP请求和响应，可以避免重复建立TCP连接导致效率低下。

    - 实现长连接主要需要以下两步： 

        - 首先要将默认的blocking I/O设为non-blocking I/O，此时read有数据则读取并返回读取的字节数，没数据则返回-1并设置errno为EAGAIN，表示下次有数据来时再读取，这么做的目的是防止长连接下数据未读取完被一直阻塞住（长连接下服务端read并不知道下一次请求会在什么时候到来）。

        - 设置epoll为ET模式（边缘触发模式），只有当状态发生变化时才会被epoll_wait返回，其他时候不会重复返回。所以长连接情况下不会因为数据未读完而每次都返回，这么做的目的是为了提高系统效率，避免每次重复检查大量处于就绪中但并没有数据的描述符。

3. epoll底层实现原理及核心参数？

    - 执行epoll_create时创建红黑树和就绪链表，执行epoll_ctl时，如果增加的句柄已经在红黑树中则立即返回，不存在则添加到描述符组成的红黑树中并向内核注册相应的回调函数。当事件到来时向就绪链表插入描述符及回调信息。

    - epoll有两种工作模式，分别是ET和LT，默认为LT，在LT下只要事件read/write等未完全处理完，每次epoll_wait时都会返回，ET模式则只在状态发生变化时返回。另外epoll_wait的第四个参数为超时时间，这里设为-1时，若无事件发生则会一直阻塞住，但若设置好timeout，每隔timeout(ms)就会被唤醒一次并返回0表示超时。

4. 定时器是做什么的，怎么实现的？

    - 初始化优先队列结构（小根堆），堆头为expire值最小的节点。

    - 新任务添加时设置超时时间（expire = add_time + timeout）。

    - 每次大循环先通过find_time函数得到最早超时请求的剩余时间（timer = expire - find_time）。 

    - 将timer填入epoll_wait的timeout位置，timer时间后自动唤醒并通过handle_expire_timers处理超时连接。

5. 如何实现线程池的？

    - 初始化线程池，创建worker线程。

    - 各worker最外层为while循环，获得互斥锁的线程可以进入线程池，若无task队列为空则 pthread_cond_wait自动解锁互斥量，置该线程为等待状态并等待条件触发。若存在task则取出队列第一个任务，**之后立即开锁，之后再并执行具体操作**。这里若先执行后开锁则在task完成前整个线程池处于锁定状态，其他线程不能取任务，相当于串行操作！

    - 建立连接后，当客户端请求到达服务器端，创建task任务并添加到线程池task队列尾，当添加完task之后调用pthread_cond_signal唤醒因没有具体task而处于等待状态的worker线程。

6. 设置handle_for_sigpipe的目的是什么？

    - 在使用Webbench时，测试完服务器总是会挂掉，阅读Webbench源码后发现当到达设定的时间之后Webbench采取的措施是直接关闭socket而不会等待最后数据的到来。这样就导致服务器在向已关闭的socket写数据，系统发送SIGPIPE信号终止了服务器。handle_for_sigpipe函数的目的就是把SIGPIPE信号的handler设置为SIG_IGN（忽略）而非终止服务器运行。

7. 怎样才算是完整的请求处理过程？

    注：EAGAIN发生于非阻塞I/O中，当做read操作却没有数据时，会设errno为EAGAIN，表示当前没有数据，稍候再试。

    - 错误断开连接情况：

        - 发生非EAGAIN错误。

        - 解析请求失败。

    - 正常断开连接情况：

        - 执行结束且为非持久连接。

    - 重新返回循环：

        - 数据未读完。

    - 更新定时器、epoll注册信息：

        - 发生EAGAIN问题。

    

## 推荐阅读

[高性能网络编程 1](http://www.52im.net/thread-560-1-1.html)

[高性能网络编程 2](http://www.cocoachina.com/bbs/read.php?tid-1705273.html)

[如何处理C10K问题](https://www.oschina.net/translate/c10k)