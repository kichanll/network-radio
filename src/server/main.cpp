#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <arpa/inet.h>

#include "thr_list.h"
#include "thr_channel.h"
#include "server_conf.h"
#include <proto.h>
#include "medialib.h"

int serversd;
struct sockaddr_in sndaddr;

struct server_conf_st server_conf = {
	/*.rcvport = */DEFAULT_RCVPORT,
	/*.mgroup = */DEFAULT_MGROUP,
	/*media_dir = */DEFAULT_MEDIADIR,
	/*.ifname = */DEFAULT_IF,
	/*.runmode = */run_daemon
};

static struct mlib_listentry_st* list;
static int list_size;

static void daemon_exit(int s){
	thr_channel_destroyall();
	thr_list_destroy();
//	mlib_freechnlist(list);
	if(s<0){
		syslog(LOG_INFO, "Daemon failure exit.");
		closelog();
		exit(1);
	}
	syslog(LOG_INFO, "Signal-%d caught, exit now.", s);
	closelog();
	exit(0);
}

static void daemonize(void){
	int fd;
	pid_t pid;
	pid = fork();
	if(pid>0){
		exit(0);
	}
	fd = open("/dev/null", O_RDWR);
	if(fd<0){
		syslog(LOG_ERR, "open(): %s", strerror(errno));
		exit(1);
	}
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	if(fd>2)close(fd);
	setsid();
	syslog(LOG_INFO, "Daemon initialized OK.");
	chdir("/");
	umask(0);
	return;
	setsid();
}

static int socket_init(void){
	struct ip_mreqn mreq;

	serversd = socket(AF_INET, SOCK_DGRAM, 0);
	if(serversd < 0){
		syslog(LOG_ERR, "socket(): %s", strerror(errno));
		exit(1);
	}

	inet_pton(AF_INET, server_conf.mgroup, &mreq.imr_multiaddr);
	inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
	mreq.imr_ifindex = if_nametoindex(server_conf.ifname);
	if(setsockopt(serversd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq))<0){
		syslog(LOG_ERR, "setsockopt(,IPPROTO_IP, IP_MULTICAST_IF,): %s", strerror(errno));
		exit(1);
	}
	sndaddr.sin_family = AF_INET;
	sndaddr.sin_port = htons(atoi(server_conf.rcvport));
	inet_pton(AF_INET, server_conf.mgroup ,&sndaddr.sin_addr);
}

/*
 *  -M set mgroup
 *  -P set rcvport
 *  -D set media dir
 *  -I NIC NAME
 *  -F set run foreground
 *  -f LOG FACILITY
 *  -H help
 */

int main(int argc, char* argv[]){
	int i, c, err;

	signal(SIGTERM, daemon_exit);
	signal(SIGINT, daemon_exit);
	signal(SIGQUIT, daemon_exit);

	openlog("netradio", LOG_PID | LOG_PERROR, LOG_DAEMON);

	while(1){
		c = getopt(argc, argv, "M:P:D:FH");
		if(c<0){
			break;
		}
		switch(c){
			case 'M':
				server_conf.mgroup = optarg;
				break;
			case 'I':
				server_conf.ifname = optarg;
				break;
			case 'P':
				server_conf.rcvport = optarg;
				break;
			case 'D':
				server_conf.media_dir = optarg;
				break;
			case 'F':
				server_conf.runmode = run_foreground;
				break;
		}
	}

	if(server_conf.runmode == run_daemon){
		daemonize();
	}
	else if(server_conf.runmode == run_foreground){
		/*Do nothing*/
	}
	else{
		syslog(LOG_ERR, "Invaild server_conf.tunmode");
		exit(1);
	}

	socket_init();

	err = mlib_getchnlist(&list, &list_size);
	if(err){
		syslog(LOG_ERR, "mlib_getchnlist() %s", strerror(err));
		exit(1);
	}
	syslog(LOG_DEBUG, "mlib_getchnlist(): list_size=%d", list_size);

	for(i=0;i<list_size;++i){
		printf("CHN:%d %s\n", list[i].id, list[i].desc);
	}
//	exit(0);		//testing

	thr_list_create(list, list_size);

	for(i=0;i<list_size;++i){
		thr_channel_create(list+i);
	}
	syslog(LOG_DEBUG, "%d channel threads created.", i);

	while(1)pause();
	exit(0);
	return 0;
}
