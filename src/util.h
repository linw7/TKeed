/* 
 * Copyright, TeeKee
 * Date, 2017.7.20
 */

#ifndef UTIL_H
#define UTIL_H

#define LISTENQ 1024
#define BUFLEN 8192
#define DELIM "="

#define TK_CONF_OK 0
#define TK_CONF_ERROR -1

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct tk_conf{
	void *root;
	int port;
	int thread_num;
}tk_conf_t;

int read_conf(char *filename, tk_conf_t* conf, char *buff, int buff_len);

int socket_bind_listen(int port);

int make_socket_non_blocking(int fd);

#endif
