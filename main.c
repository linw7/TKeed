/* 
 * Copyright, TeeKee
 * Date, 2017.7.20
 */

// 系统头文件
#include <stdio.h>
#include <string>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
// 封装并实现新功能
#include "epoll.h"
#include "threadpool.h"
// 本地功能
#include "timer.h"
#include "http.h"
#include "util.h"

#define DEFAULT_CONFIG "tkeed.conf"
#define VERSION "0.1"

extern struct epoll_event *events;

static const struct option options{
	{"help", no_argument, NULL, '?'},
    {"version", no_argument, NULL, 'V'},
    {"conf", required_argument, NULL, 'c'},
    {NULL,0,NULL,0}
};

static void usage_man(){
	fprintf(stderr,
	"TKeed [option]... \n"
	"  -c|--conf <config file>  Default ./tkeed.conf.\n"
	"  -?|-h|--help             This information.\n"
	"  -v|--version             Display program version.\n"
	);
}

int main(int argc, char *argv[]){
    int option = 0;
	int options_index = 0;
	char *conf_file = DEFAULT_CONFIG;
	if(argc == 1){
		usage_man();
		return 0;
	}
	while((opt = getopt_long(argc, argv, "vc:?h", option, &options_index)) != EOF){
		switch(opt){
			case 'c' : 
				conf_file = optarg;
				break;
			case 'v' :
				printf(VERSION);
				break;
			case 'h':
				usage_man();
			default:
				usage_man();
		}
	}

	debug("conf_file = %s", conf_file);

	if(optind < argc){
		log_err("non-option : ");
		while(optind < argc)
			log_err("%s", argv[optind++]);
		return 0;
	}

	char conf_buff[BUFLEN];
	tk_conf_t conf;
	int read_conf_status = read_conf(conf_file, &conf, conf_buff, BUFLEN);
	check(read_conf_status == TK_CONF_OK, "read config error");

	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	if(sigaction(SIGPIPE, &sa, NULL)){
		log_err("install signal handler for SIGPIPE failed");
		return 0;
	}

	int listen_fd = socket_bind_listen(conf.port);
	int rc = make_socket_non_blocking(listen_fd);
}