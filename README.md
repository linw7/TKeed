# TKeed WebServer

> 麻雀虽小，五脏俱全。

---

## 进度记录

| Scrum | Feature | Done | Srpint Review |
|:-----:|:-------:|:----:|:-------------:|
| Sprint 1|配置读取；监听请求；epoll封装| 编译通过；单元测试通过|调试比较困难|
| Sprint 2 |改变封装粒度；定义常量值|对Sprint 1封装；规定常量| 代码规范有待加强|
| Sprint 3 |定义tk_time_t结构体；实现时间部分操作|不能编译，需要用优先队列组织 |定义结构体需考虑扩展性|
| Sprint 4 |定义tk_pq_t结构体，完成优先队列 | 未定义类型指针用(void *)代替可通过编译| 需要加入debug模块辅助调试|
| Sprint 5 |完成debug模块；封装IO操作 | debug接口可用，RIO包编译通过|debug接口通用性较差|

---

## 主要模块

| Module | Component | Efficiency |Dependent|Line|
|:-----:|:-------:|:----:|:-------:|:------:|
|主模块  |  main.c | 全局入口，完成初始化，循环调用accept和do_request |util.h + epoll.h + http.h + timer.h| 73 | 
|工具模块|util.h & util.c|读取配置文件，创建套接字并绑定监听，分发epoll事件 | epoll.h + http_request.h | 32 + 173 |
|epoll模块|epoll.h & epoll.c|对epoll接口封装并实现功能分发| -- | -- | -- |
|time模块| timer.h & timer.c|记录时间戳，设置超时控制| -- | -- | -- |
|http模块| http.h & http.c | -- | -- | -- |
|http_parse模块|http_parse.h & http_parse.c | -- | -- | -- |
|http_request模块|http_request.h & http_request.c | -- | -- | -- |
|list模块（工具）| list.h & list.c | -- | -- | -- |
|pq模块（工具）| priority_queue.h & priority_queue.c | -- | -- | -- |
|rio模块（工具）|rio.h & rio.c | -- | -- | -- |


---

## 开发环境

于Linux环境下开发，前期开发编辑器为Sublime，前期无法完整调试，逐个模块编译测试是否有语法错误、链接错误。功能测试只有工具模块priority_queue和list可以单独用Mock数据跑通，util模块可提前和主模块链接并调试，其他模块需要或多或少有依赖关系，中后期可测试。测试中能够通过debug信息定位的错误直接修改，难以定位的错误用Clion单步调试功能。

- 操作系统：Ubuntu 16.04

- 编辑器：Sublime + Vim

- 编译器：gcc 5.4.0

- 单元测试：~~gtest~~

- 版本控制：git

- 宏观结构：[callgraph](http://www.tinylab.org/callgraph-draw-the-calltree-of-c-functions/)

- 集成环境：CLion

---

## 项目目的

- 软件开发流程

    遵循完整开发流程，确定需求 -> 选定服务器模型 -> 定义数据结构 -> 开发辅助工具 -> 单元测试 -> 核心部分开发 -> 集成测试 -> 性能测试。开发环境也统一到Linux环境下，通过git进行版本控制，尽可能模式真实工作环境。

- 基础知识

    开发HTTP服务器从宏观上来说会对网络协议TCP及其各个状态理解更深，会对HTTP协议主要字段的功能理解更深，会对操作系统中多线程、多进程并发概念和局限性理解更深刻，会对网络I/O模型认识更深。

- 数据结构

    通过对场景需求和将来扩展性的研究，需要设计合理的、高效的数据结构，比如在本项目中最核心的tk_request_t、tk_timer_t和tk_pq_t结构考虑到了扩展性和操作高效性（比如tk_time_t中的deleted字段、tk_pq_t中的size字段）。同时根据需求实现了list和priority_queue库并提供统一接口，可以帮助更好地掌握数据结构和设计接口。

- 编程语言

    项目中涉及C语言中方方面面，比如预定义、typedef、全局变量、静态全局变量、函数指针、位运算、强制类型转换、结构体操作等，很多在调试时候遇到过问题，但也能学到很多。另外，也顺带学些了很多编译和调试的小技巧。

- 开发工具

    最后，在开发过程中使用到的都是最基本、最常用的开发工具，开发、调试、版本控制都有所涉及，可以更好地利用辅助工具完成开发任务。
---

## 背景知识

- 基本原理

    - 网络基础

        - 网络模型

        - 应用层

        - 传输层

        - 网络层

    - 客户端 - 服务器模型

    - 套接字接口

        - 客户端

        - 服务器端

- 实现方案比较

    - 迭代服务器

    - 基于多进程的服务器

    - 基于多线程的服务器

    - 基于事件驱动的服务器

- 具体实现

    - 迭代服务器

    - 基于多进程的服务器

    - 基于多线程的服务器

    - 基于事件驱动的服务器

---

[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)
