//
// Latest edit by TeeKee on 2017/7/23.
//

#include <errno.h>
#include "http.h"

static const char* get_file_type(const char *type);
static void parse_uri(char *uri, int length, char *filename, char *query);
static void do_error(int fd, char *cause, char *err_num, char *short_msg, char *long_msg);
static void serve_static(int fd, char *filename, size_t filesize, tk_http_out_t *out);

static char *ROOT = NULL;

mime_type_t tkeed_mime[] = 
{
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/msword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"},
    {NULL ,"text/plain"}
};

static void parse_uri(char *uri_start, int uri_length, char *filename, char *query){
    uri_start[uri_length] = '\0';
    // 找到'?'位置界定非参部分
    char *delim_pos = strchr(uri_start, '?');
    int filename_length = (delim_pos != NULL) ? ((int)(delim_pos - uri_start)) : uri_length;
    strcpy(filename, ROOT);
    // 将uri中属于'?'之前部分内容追加到filename
    strncat(filename, uri_start, filename_length);
    // 在请求中找到最后一个'/'位置界定文件位置
    char *last_comp = strrchr(filename, '/');
    // 在文件名中找到最后一个'.'界定文件类型
    char *last_dot = strrchr(last_comp, '.');
    // 请求文件时末尾加'/'
    if((last_dot == NULL) && (filename[strlen(filename) - 1] != '/')){
        strcat(filename, "/");
    }
    // 默认请求index.html
    if(filename[strlen(filename) - 1] == '/'){
        strcat(filename, "index.html");
    }
    return; 
}

const char* get_file_type(const char *type){
    // 将type和索引表中后缀比较，返回类型用于填充Content-Type字段
    for(int i = 0; tkeed_mime[i].type != NULL; ++i){
        if(strcmp(type, tkeed_mime[i].type) == 0)
            return tkeed_mime[i].value;
    }
    // 未识别返回"text/plain"
    return "text/plain";
}

// 响应错误信息
void do_error(int fd, char *cause, char *err_num, char *short_msg, char *long_msg){
    // 响应头缓冲（512字节）和数据缓冲（8192字节）
    char header[MAXLINE];
    char body[MAXLINE];

    // 用log_msg和cause字符串填充错误响应体
    sprintf(body, "<html><title>TKeed Error<title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\n", body);
    sprintf(body, "%s%s : %s\n", body, err_num, short_msg);
    sprintf(body, "%s<p>%s : %s\n</p>", body, long_msg, cause);
    sprintf(body, "%s<hr><em>TKeed web server</em>\n</body></html>", body);

    // 返回错误码，组织错误响应头
    sprintf(header, "HTTP/1.1 %s %s\r\n", err_num, short_msg);
    sprintf(header, "%sServer: TKeed\r\n", header);
    sprintf(header, "%sContent-type: text/html\r\n", header);
    sprintf(header, "%sConnection: close\r\n", header);
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, (int)strlen(body));

    // Add 404 Page

    // 发送错误信息
    rio_writen(fd, header, strlen(header));
    rio_writen(fd, body, strlen(body));
    return;
}

// 处理静态文件请求
void serve_static(int fd, char *filename, size_t filesize, tk_http_out_t *out){
    // 响应头缓冲（512字节）和数据缓冲（8192字节）
    char header[MAXLINE];
    char buff[SHORTLINE];
    struct tm tm;
    
    // 返回响应报文头，包含HTTP版本号状态码及状态码对应的短描述
    sprintf(header, "HTTP/1.1 %d %s\r\n", out->status, get_shortmsg_from_status_code(out->status));
    
    // 返回响应头
    // Connection，Keep-Alive，Content-type，Content-length，Last-Modified
    if(out->keep_alive){
        // 返回默认的持续连接模式及超时时间（默认500ms）
        sprintf(header, "%sConnection: keep-alive\r\n", header);
        sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, TIMEOUT_DEFAULT);
    }
    if(out->modified){
        // 得到文件类型并填充Content-type字段
        const char* filetype = get_file_type(strrchr(filename, '.'));
        sprintf(header, "%sContent-type: %s\r\n", header, filetype);
        // 通过Content-length返回文件大小
        sprintf(header, "%sContent-length: %zu\r\n", header, filesize);
        // 得到最后修改时间并填充Last-Modified字段
        localtime_r(&(out->mtime), &tm);
        strftime(buff, SHORTLINE,  "%a, %d %b %Y %H:%M:%S GMT", &tm);
        sprintf(header, "%sLast-Modified: %s\r\n", header, buff);
    }
    sprintf(header, "%sServer : TKeed\r\n", header);

    // 空行（must）
    sprintf(header, "%s\r\n", header);

    // 发送报文头部并校验完整性
    size_t send_len = (size_t)rio_writen(fd, header, strlen(header));
    if(send_len != strlen(header)){
        perror("Send header failed");
        return;
    }

    // 打开并发送文件
    int src_fd = open(filename, O_RDONLY, 0);
    char *src_addr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, src_fd, 0);
    close(src_fd);

    // 发送文件并校验完整性
    send_len = rio_writen(fd, src_addr, filesize);
    if(send_len != filesize){
        perror("Send file failed");
        return;
    }
    munmap(src_addr, filesize);
}

int error_proess(struct stat* sbufptr, char *filename, int fd){
    // 处理文件找不到错误
    if(stat(filename, sbufptr) < 0){
        do_error(fd, filename, "404", "Not Found", "TKeed can't find the file");
        return 1;
    }

    // 处理权限错误
    if(!(S_ISREG((*sbufptr).st_mode)) || !(S_IRUSR & (*sbufptr).st_mode)){
        do_error(fd, filename, "403", "Forbidden", "TKeed can't read the file");
        return 1;
    }

    return 0;
}

void do_request(void* ptr){
    tk_http_request_t* request = (tk_http_request_t*)ptr;
    int fd = request->fd;
    ROOT = request->root;
    char filename[SHORTLINE];
    struct stat sbuf;
    int rc, n_read;
    char* plast = NULL;
    size_t remain_size;

    tk_del_timer(request);

    while(1){
        // plast指向缓冲区buf当前可写入的第一个字节位置，这里取余是为了实现循环缓冲
        plast = &request->buff[request->last % MAX_BUF];

        // remain_size表示缓冲区当前剩余可写入字节数
        remain_size = MIN(MAX_BUF - (request->last - request->pos) - 1, MAX_BUF - request->last % MAX_BUF);

        // 从连接描述符fd读取数据并复制到用户缓冲区plast指向的开始位置
        n_read = read(fd, plast, remain_size);

        // 已读到文件尾或无可读数据，断开连接
        if(n_read == 0)
            goto err;

        // 非EAGAIN错误，断开连接
        if(n_read < 0 && (errno != TK_AGAIN))
            goto err;

        // Non-blocking下errno返回EAGAIN则重置定时器（进入此循环表示连接被激活），重新注册，在不断开TCP连接情况下重新等待下一次用户请求
        if((n_read < 0) && (errno == TK_AGAIN))
            break;

        // 更新读到的总字节数
        request->last += n_read;

        // 解析请求报文行
        rc = tk_http_parse_request_line(request);
        if(rc == TK_AGAIN)
            continue;
        else if(rc != 0)
            goto err;

        // 解析请求报文体
        rc = tk_http_parse_request_body(request);
        if(rc == TK_AGAIN)
            continue;
        else if(rc != 0)
            goto err;

        // 分配并初始化返回数据结构
        tk_http_out_t* out = (tk_http_out_t *)malloc(sizeof(tk_http_out_t));
        tk_init_out_t(out, fd);

        // 解析URI，获取文件名
        parse_uri(request->uri_start, request->uri_end - request->uri_start, filename, NULL);

        // 处理相应错误
        if(error_proess(&sbuf, filename, fd))
            continue;

        tk_http_handle_header(request, out);

        // 获取请求文件类型
        out->mtime = sbuf.st_mtime;

        // 处理静态文件请求
        serve_static(fd, filename, sbuf.st_size, out);

        // 释放返回数据结构
        free(out);

        // 处理HTTP长连接，控制TCP是否断开连接
        if (!out->keep_alive)
            goto close;
    }
    // 一次请求响应结束后不直接断开TCP连接，而是重置状态
    // 修改已经注册描述符的事件类型
    // 重置定时器，每等待下一次请求均生效
    tk_epoll_mod(request->epoll_fd, request->fd, request, (EPOLLIN | EPOLLET | EPOLLONESHOT));
    tk_add_timer(request, TIMEOUT_DEFAULT, tk_http_close_conn);

    // 每完成一次请求数据的响应都会return以移交出worker线程使用权，并在未断开TCP连接情况下通过epoll监听下一次请求
    return;

    err:
    close:
    // 发生错误或正常关闭
    // 关闭相应连接描述符，释放用户请求数据结构
    tk_http_close_conn(request);
}