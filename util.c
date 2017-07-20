/* 
 * Copyright, TeeKee
 * Date, 2017.7.20
 */


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include "util.h"
#include "dbg.h"

int read_conf(char *filename, tk_conf_t* conf, char *buff, int buff_len){
	// 以只读方式打开文件
	FILE *fp = fopen(filename, "r");
	if(!fp) {
		log_err("cannot open user config file : %s", filename);
		return TK_CONF_ERROR;
	}

	char *curr_pos = buff;
	char *delim_pos= buff;
	int pos = 0;
	int line_len = 0;
	while(fgets(curr_pos, buff_len - pos, fp)){
		// 定位每行第一个界定符位置
		delim_pos = strstr(curr_pos, DELIM);
		if(!delim_pos){
			log_err("cannot find the DELIM position");
			return TK_CONF_ERROR;
		}
		if(curr_pos[strlen(curr_pos) - 1] == '\n'){
			curr_pos[strlen(curr_pos) - 1] = '\0'
		}
		// 得到root信息
		if(strncmp("root", curr_pos, 4) == 0){
			conf->root = delim_pos + 1;
		}
		// 得到port值
		if(strncmp("port", curr_pos, 4) == 0){
			conf->port = atoi(delim_pos + 1);
		} 
		// 得到thread数量
		if(strcmp("thread_num", curr_pos, 9) == 0){
			conf->thread_num = atoi(delim_pos + 1);
		}
		// line_len得到当前行行长
		line_len = strlen(curr_pos);
		// 当前位置跳转至下一行首部
		curr_pos += line_len;
	}
	fclose(fp);
	return TK_CONF_OK;
}

int socket_bind_listen(int port){
	// 检查port值，取正确区间范围
	port = ((port <= 1024) || (port >= 65535)) ? 6666 : port;

	// 创建socket(IPv4 + TCP)，返回监听描述符
	int listen_fd = 0;
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket error");
		return -1;
	}

	// 消除bind时"Address already in use"错误
	if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) == -1){
		perror("setsockopt error");
		return -1;
	}

	// 设置服务器IP和Port，和监听描述副绑定
	struct sockaddr_in server_addr;
	bzero((char *)server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons((unsigned short)port);
	if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) = 0){
		perror("bind error");
		return -1;
	}

	// 开始监听，最大等待队列长为LISTENQ
	if(listen(listen_fd, LISTENQ) < 0){
		perror("listen");
		return -1;
	}
	return listen_fd;
}

int make_socket_non_blocking(int fd){
	int flag = fcntl(fd, F_GETFL, 0);
	if(flag == -1){
		log_err("fcntl");
		return -1;
	}
	flag |= O_NONBLOCK;
	if(fcntl(fd, F_SETFL, flag) == -1){
		log_err("fcntl");
		return -1;
	}
}
