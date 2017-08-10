# TKeed WebServer

**TKeed is a high performance HTTP WebServer uses the Reactor model. Code normative and functional scalability are close to the industry level. The project will be updated until feature have been completed. Have a fun. SYSU, TeeKee.**

## Dev Document

| Part Ⅰ | Part Ⅱ | Part Ⅲ | Part Ⅳ | Part Ⅴ | Part Ⅵ | Part Ⅷ | Part Ⅸ | Part Ⅹ |
| :--------: | :---------: | :---------: | :---------: | :---------: | :---------: |:--------:| :--------:|:--------:|
|  [项目目的](https://github.com/linw7/TKeed/blob/master/%E9%A1%B9%E7%9B%AE%E7%9B%AE%E7%9A%84.md)  | [并发模型](https://github.com/linw7/TKeed/blob/master/%E5%B9%B6%E5%8F%91%E6%A8%A1%E5%9E%8B.md)|[核心结构](https://github.com/linw7/TKeed/blob/master/%E6%A0%B8%E5%BF%83%E7%BB%93%E6%9E%84%E4%BD%93.md)|[整体架构](https://github.com/linw7/TKeed/blob/master/%E6%9E%B6%E6%9E%84%E5%88%86%E6%9E%90.md)|  [主要函数](https://github.com/linw7/TKeed/blob/master/%E4%B8%BB%E8%A6%81%E5%87%BD%E6%95%B0.md)| [遇到的困难](https://github.com/linw7/TKeed/blob/master/%E5%90%AF%E7%A4%BA%E5%BD%95.md) |  [测试及改进](https://github.com/linw7/TKeed/blob/master/%E6%B5%8B%E8%AF%95%E5%8F%8A%E6%94%B9%E8%BF%9B.md) | [背景知识](https://github.com/linw7/TKeed/blob/master/%E8%83%8C%E6%99%AF%E7%9F%A5%E8%AF%86.md)|[使用教程](https://asciinema.org/a/132577)|

---

## Dev Environment

**Dev Tool**

- 操作系统：Ubuntu 16.04

- 编辑器：Sublime + Vim

- 编译器：gcc 5.4.0

- 单元测试：~~gtest~~

- 版本控制：git

- 代码结构：[Understand](https://scitools.com/) + [callgraph](http://blog.csdn.net/solstice/article/details/488865)

- 集成环境：[Clion](https://www.jetbrains.com/clion/)

**Other**

- 自动化构建：[Travis CI](https://travis-ci.org/linw7/TKeed)

- 压测工具：[WebBench](https://github.com/EZLippi/WebBench)

--- 

## Timeline

**Now**

- v1.0已经完成，本地已调试通过。提交到GitHub上的代码会由Travis自动构建。

    特性：

    - 添加Timer定时器，定时回调handler处理超时请求

        - 高效的小根堆结构

        - 惰性删除方式

    - 实现了HTTP长连接传输数据

        - 非阻塞I/O

        - epoll边缘触发模式（ET）

    - 线程池操作及其同步互斥管理

        - 调度选项

            - 队列式FIFO调度模式

            - 加入优先级的优先队列 (+)

    - 使用状态机解析HTTP协议，非简单字符串匹配方式解析请求

- v1.1修改了CPU负载较高问题，修改后1000并发各线程（4worker）CPU使用率10%左右。

**Feature**

- v2.0实现Json解释器解析配置

- v3.0实现FastCGI（功能扩展）

- v4.0实现服务器缓存（性能加速）

- v5.0实现反向代理（安全性及负载均衡）

---

[![Build Status](https://travis-ci.org/linw7/TKeed.svg?branch=master)](https://travis-ci.org/linw7/TKeed)
[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)

---