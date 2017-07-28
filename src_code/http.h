//
// Latest edit by TeeKee on 2017/7/23.
//

#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "timer.h"
#include "util.h"
#include "rio.h"
#include "epoll.h"
#include "http_parse.h"
#include "http_request.h"

#define MAXLINE 8192
#define SHORTLINE 512

#define tk_str3_cmp(m, c0, c1, c2, c3)                                       \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define tk_str3Ocmp(m, c0, c1, c2, c3)                                       \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define tk_str4cmp(m, c0, c1, c2, c3)                                        \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

// 用key-value表示mime_type_t
typedef struct mime_type{
    const char *type;
    const char *value;
}mime_type_t;

void do_request(void *ptr);

#endif