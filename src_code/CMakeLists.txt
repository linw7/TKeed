cmake_minimum_required(VERSION 3.7)
project(src)

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
        timer.c
        timer.h
        util.c
        util.h struct.h)

add_executable(src ${SOURCE_FILES})