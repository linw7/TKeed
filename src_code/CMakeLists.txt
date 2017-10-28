cmake_minimum_required(VERSION 3.8)
project(src_code)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        epoll.c
        epoll.h
        error.h
        http.c
        http.h
        http_parse.c
        http_parse.h
        http_request.c
        http_request.h
        list.h
        main.c
        priority_queue.c
        priority_queue.h
        rio.c
        rio.h
        threadpool.c
        threadpool.h
        timer.c
        timer.h
        util.c
        util.h)

add_executable(src_code ${SOURCE_FILES})