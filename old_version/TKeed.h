#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/* 默认文件权限被设置为DEF_MODE & ~DEF_UMASK */
#define DEF_MODE   S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
#define DEF_UMASK  S_IWGRP|S_IWOTH

/* 简化bind(), connect()和accept()函数调用 */
typedef struct sockaddr SA;

/* 外部环境 */
extern int h_errno;     /* 定义了bind错误 */
extern char **environ;  /* 通过libc定义 */

/* 基本参数 */
#define MAXLINE 8192   /* 最大文本行长度 */
#define MAXBUF 8192    /* 最大IO缓冲大小 */
#define LISTENQ 1024   /* listen()函数第二个参数 */

/* 错误处理函数 */
void unix_error(char *msg);
void posix_error(int code, char *msg);
void dns_error(char *msg);
void gai_error(int code, char *msg);
void app_error(char *msg);

/* 进程控制封装 */
pid_t Fork(void);
void Execve(const char *filename, char *const argv[], char *const envp[]);
pid_t Wait(int *status);
pid_t Waitpid(pid_t pid, int *iptr, int options);
void Kill(pid_t pid, int signum);
unsigned int Sleep(unsigned int secs);
void Pause(void);
unsigned int Alarm(unsigned int seconds);
void Setpgid(pid_t pid, pid_t pgid);
pid_t Getpgrp();

/* 信号封装 */
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
void Sigemptyset(sigset_t *set);
void Sigfillset(sigset_t *set);
void Sigaddset(sigset_t *set, int signum);
void Sigdelset(sigset_t *set, int signum);
int Sigismember(const sigset_t *set, int signum);
int Sigsuspend(const sigset_t *set);

/* UNIX I/O封装 */
int Open(const char *pathname, int flags, mode_t mode);
ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);
off_t Lseek(int fildes, off_t offset, int whence);
void Close(int fd);
int Select(int  n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout);
int Dup2(int fd1, int fd2);
void Stat(const char *filename, struct stat *buf);
void Fstat(int fd, struct stat *buf) ;

/* 目录封装 */
DIR *Opendir(const char *name);
struct dirent *Readdir(DIR *dirp);
int Closedir(DIR *dirp);

/* 存储器映射封装 */
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void Munmap(void *start, size_t length);

/* 动态存储分配封装 */
void *Malloc(size_t size);
void *Realloc(void *ptr, size_t size);
void *Calloc(size_t nmemb, size_t size);
void Free(void *ptr);

/* 标准I/O封装 */
void Fclose(FILE *fp);
FILE *Fdopen(int fd, const char *type);
char *Fgets(char *ptr, int n, FILE *stream);
FILE *Fopen(const char *filename, const char *mode);
void Fputs(const char *ptr, FILE *stream);
size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* 套接字接口封装 */
int Socket(int domain, int type, int protocol);
void Setsockopt(int s, int level, int optname, const void *optval, int optlen);
void Bind(int sockfd, struct sockaddr *my_addr, int addrlen);
void Listen(int s, int backlog);
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);
void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen);

/* 协议操作 */
void Getaddrinfo(const char *node, const char *service,
                 const struct addrinfo *hints, struct addrinfo **res);
void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
                 size_t hostlen, char *serv, size_t servlen, int flags);
void Freeaddrinfo(struct addrinfo *res);
void Inet_ntop(int af, const void *src, char *dst, socklen_t size);
void Inet_pton(int af, const char *src, void *dst);

/* DNS封装 */
struct hostent *Gethostbyname(const char *name);
struct hostent *Gethostbyaddr(const char *addr, int len, int type);


/* RIO读缓冲区格式 */
#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd;                  /* buf的描述符 */
    int rio_cnt;                 /* buf中尚未使用字节数 */
    char *rio_bufptr;            /* 指向下一个尚未使用缓冲的指针 */
    char rio_buf[RIO_BUFSIZE];   /* 定义缓冲 */
}rio_t;
/* rio包 */
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);

void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/* RIO包封装 */
ssize_t Rio_readn(int fd, void *usrbuf, size_t n);
void Rio_writen(int fd, void *usrbuf, size_t n);

void Rio_readinitb(rio_t *rp, int fd);
ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/* 独立于协议的客户、服务器操作 */
int open_clientfd(char *hostname, char *port);
int open_listenfd(char *port);

/* 封装独立于协议的客户、服务器操作 */
int Open_clientfd(char *hostname, char *port);
int Open_listenfd(char *port);


/* 错误处理 */
void unix_error(char *msg)
{
    fprintf(stderr, "%s : %s\n", msg, strerror(errno));
    exit(0);
}

void posix_error(int code, char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

void dns_error(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

void gai_error(int code, char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    exit(0);
}

void app_error(char *msg) /* Application error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

/* UNIX进程控制封装 */
pid_t Fork(void)
{
    pid_t pid;
    if((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}

void Execve(const char *filename, char *const argv[], char *const envp[])
{
    if(execve(filename, argv, envp) < 0)
        unix_error("Execve error");
}

pid_t Wait(int *ststus)
{
    pid_t pid;
    if((pid = wait(ststus)) < 0)
        unix_error("Wait error");
    return pid;
}

/* UNIX I/O封装 */
int Open(const char *pathname, int flags, mode_t mode)
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
    unix_error("Open error");
    return rc;
}

ssize_t Read(int fd, void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0)
    unix_error("Read error");
    return rc;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
    unix_error("Write error");
    return rc;
}

off_t Lseek(int fildes, off_t offset, int whence)
{
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
    unix_error("Lseek error");
    return rc;
}

void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
    unix_error("Close error");
}

int Select(int  n, fd_set *readfds, fd_set *writefds,
       fd_set *exceptfds, struct timeval *timeout)
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
    unix_error("Select error");
    return rc;
}

int Dup2(int fd1, int fd2)
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
    unix_error("Dup2 error");
    return rc;
}

void Stat(const char *filename, struct stat *buf)
{
    if (stat(filename, buf) < 0)
    unix_error("Stat error");
}

void Fstat(int fd, struct stat *buf)
{
    if (fstat(fd, buf) < 0)
    unix_error("Fstat error");
}

/* 存储器映射封装 */
void *Mmap(void *addr, size_t len, int port, int flags, int fd, off_t offset)
{
    void *ptr;
    if((ptr = mmap(addr, len, port, flags, fd, offset)) == ((void *)-1))
        unix_error("mmap error");
    return ptr;
}

void Munmap(void *start, size_t length)
{
    if (munmap(start, length) < 0)
        unix_error("munmap error");
}

/* 套接字接口封装 */
int Socket(int domain, int type, int protocol)
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
        unix_error("Socket error");
    return rc;
}

void Setsockopt(int s, int level, int optname, const void *optval, int optlen)
{
    int rc;
    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
        unix_error("Setsockopt error");
}

void Bind(int sockfd, struct sockaddr *my_addr, int addrlen)
{
    int rc;
    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
        unix_error("Bind error");
}

void Listen(int s, int backlog)
{
    int rc;

    if ((rc = listen(s,  backlog)) < 0)
        unix_error("Listen error");
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
        unix_error("Accept error");
    return rc;
}

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen)
{
    int rc;
    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
        unix_error("Connect error");
}

/* 协议操作 */
void Getaddrinfo(const char *node, const char *service,
                 const struct addrinfo *hints, struct addrinfo **res)
{
    int rc;
    if ((rc = getaddrinfo(node, service, hints, res)) != 0)
        gai_error(rc, "Getaddrinfo error");
}

void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
                 size_t hostlen, char *serv, size_t servlen, int flags)
{
    int rc;
    if ((rc = getnameinfo(sa, salen, host, hostlen, serv,
                          servlen, flags)) != 0)
        gai_error(rc, "Getnameinfo error");
}

void Freeaddrinfo(struct addrinfo *res)
{
    freeaddrinfo(res);
}

void Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (!inet_ntop(af, src, dst, size))
        unix_error("Inet_ntop error");
}

void Inet_pton(int af, const char *src, void *dst)
{
    int rc;
    rc = inet_pton(af, src, dst);
    if (rc == 0)
        app_error("inet_pton error: invalid dotted-decimal address");
    else if (rc < 0)
        unix_error("Inet_pton error");
}

/* DNS封装 */
struct hostent *Gethostbyname(const char *name)
{
    struct hostent *p;
    if ((p = gethostbyname(name)) == NULL)
        dns_error("Gethostbyname error");
    return p;
}

struct hostent *Gethostbyaddr(const char *addr, int len, int type)
{
    struct hostent *p;

    if ((p = gethostbyaddr(addr, len, type)) == NULL)
        dns_error("Gethostbyaddr error");
    return p;
}

/* rio包实现 */
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    /* 无缓冲read */
    /* 从描述符fd当前位置最多传送n个字节到存储器位置usrbuf */
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    while(nleft > 0){
        /* 若成功则为读的字节数，若EOF则为0，出错为-1 */
        if((nread = read(fd, bufp, nleft)) < 0){
            if(errno == EINTR)
                nread = 0;
            else
                return -1;
        }
        else if(nread == 0)
            break;
        nleft -= nread;
        bufp += nread;
    }
    /* 返回已经读取的字节数 */
    return n - nleft;
}

ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    /* 无缓冲write */
    /* 从位置usrbuf传送n个字节到描述符fd */
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;
    while (nleft > 0) {
        /* 若成功则为写的字节数，若失败则为-1 */
        if ((nwritten = write(fd, bufp, nleft)) <= 0){
            if (errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

void rio_readinitb(rio_t *rp, int fd)
{
    /* 对rio_t类型变量做初始化 */
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;
    while(rp->rio_cnt <= 0){
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if(rp->rio_cnt < 0){
            if(errno != EINTR)
                return -1;
        }
        else if(rp->rio_cnt == 0)
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;
    }
    cnt = n;
    /* 实际读取小于请求读取的字节数 */
    if(rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    /* 将缓冲中实际得到的数据拷贝到usrbuf指向的空间 */
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    /* 返回实际读取字节数（无差错返回n，出错为-1） */
    return cnt;
}

ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    while (nleft > 0){
        /* rio_read将rio_buf中读取内容拷贝到usrbuf中，返回实际得到字节数 */
        if ((nread = rio_read(rp, bufp, nleft)) < 0)
            return -1;
        else if (nread == 0)
            break;
        nleft -= nread;
        bufp += nread;
    }
    /* 返回得到的字节数 */
    return (n - nleft);
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;
    for(n = 1; n < maxlen; ++n){
        if((rc = rio_read(rp, &c, 1)) == 1){
            *bufp++ = c;
            // 若命令行得到'\n'则结束输入
            if(c == '\n')
                break;
        }
        else if(rc == 0){
            // 不含任何输入，EOF
            if(n == 1)
                return 0;
            // 第n行结束输入，EOF
            else
                break;
        }
        else
            return -1;
    }
    *bufp = '\0';
    return n - 1;
}

/* RIO包封装实现 */
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;
    if((n = rio_readn(fd, ptr, nbytes)) < 0)
        unix_error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n)
{
    if(rio_writen(fd, usrbuf, n) != n)
        unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;
    if((rc = rio_readnb(rp, usrbuf, n)) < 0)
        unix_error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;
    if((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
        unix_error("Rio_readlineb error");
    return rc;
}

/* 独立于协议的客户、服务器操作 */
int open_clientfd(char *hostname, char *port)
{
    int clientfd;
    struct addrinfo hints, *listp, *p;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_flags |= AI_ADDRCONFIG;
    Getaddrinfo(hostname, port, &hints, &listp);
    for(p = listp; p; p = p->ai_next){
        if((clientfd = Socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;
        if(connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break;
        Close(clientfd);
    }
    Freeaddrinfo(listp);
    if (!p)
        return -1;
    else
        return clientfd;
}

int open_listenfd(char *port)
{
    struct addrinfo hints, *listp, *p;
    int listenfd, optval = 1;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_flags |= AI_NUMERICSERV;
    hints.ai_flags |= AI_ADDRCONFIG;
    Getaddrinfo(NULL, port, &hints, &listp);
    for (p = listp; p; p = p->ai_next) {
        if ((listenfd = Socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;
        Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int));
        if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        Close(listenfd);
    }
    Freeaddrinfo(listp);
    if (!p)
        return -1;
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
}

/* 封装独立于协议的客户、服务器操作 */
int Open_clientfd(char *hostname, char *port)
{
    int rc;
    if ((rc = open_clientfd(hostname, port)) < 0)
        unix_error("Open_clientfd error");
    return rc;
}

int Open_listenfd(char *port)
{
    int rc;
    if ((rc = open_listenfd(port)) < 0)
        unix_error("Open_listenfd error");
    return rc;
}
