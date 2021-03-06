#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <proto.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <signal.h>

#include "client_conf.h"

static int pd[2];

struct client_conf_st client_conf = {
	/*.rcvport = */DEFAULT_RCVPORT,
	/*.mgroup = */DEFAULT_MGROUP,
	/*.player_cmd = */DEFAULT_PLAYERCMD
};

static void printhelp(void){
}

static void usr1_handler(int s){

}

/*
 *  -P,--port set port
 *  -M,--mgroup set multi-board ip
 *  -p,--player set cmd
 *  -H,--help help list
 */

static struct option argarr[] = {
	{"port", 1, NULL, 'P'},
	{"mgroup", 1, NULL, 'M'},
	{"player", 1, NULL, 'p'},
	{"help", 0, NULL, 'H'},
	{NULL, 0, NULL, 0}
};

#define NR_ARGS 4

static ssize_t writen(int fd, const char* buf, size_t len){
	int pos;
	int ret;

	pos=0;
	while(len>0){
		ret=write(fd, buf+pos, len);
		if(ret<0){
			if(errno==EINTR){
				continue;
			}
			break;
		}
		len -= ret;
		pos += ret;
	}
	if(pos==0){
		return -1;
	}
	return pos;
}

int main(int argc, char* argv[]){

	pid_t pid;
	int c;
	int sd, len, index;
	struct sockaddr_in laddr, raddr, serveraddr;
	socklen_t raddr_len, serveraddr_len;
	struct msg_list_st *msg_list;
	struct msg_channel_st *msg_channel;
	chnid_t chosenid;
	struct ip_mreqn mreq;
	int ret ,count ,child_pause=1;

	signal(SIGUSR1, usr1_handler);

	index = 0;
	while(1){
		c = getopt_long(argc, argv, "P:M:p:H", argarr, &index);
		if(c<0){
			break;
		}
		switch(c){
			case 'P':
				client_conf.rcvport = optarg;
				break;
			case 'M':
				client_conf.mgroup = optarg;
				break;
			case 'p':
				client_conf.player_cmd = optarg;
				break;
			case 'H':
				printhelp();
				exit(0);
				break;
			default:
				abort();
				break;
		}
	}

	sd = socket(AF_INET, SOCK_DGRAM, 0/*IPPROTO_UDP*/);
	if(sd<0){
		perror("socket()");
		exit(1);
	}
	fprintf(stderr, "socket() OK\n");

	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(atoi(client_conf.rcvport));
	inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
	if(bind(sd, (struct sockaddr*)&laddr, sizeof(laddr))<0){
		perror("bind()");
		exit(1);
	}
	fprintf(stderr, "bind() OK\n");

	inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);
	inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
	mreq.imr_ifindex = if_nametoindex("eth0");
	if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))<0){
		perror("setsockopt()");
		exit(1);
	}
	int val = 1;
	if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &val, 4) < 0){
		perror("setsockopt()");
		exit(1);
	}
	fprintf(stderr, "Join mgroup %s OK\n", client_conf.mgroup);

	val = 1024000;
	if(setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val))<0){
		perror("setsockopt()");
		exit(1);
	}
	
	if(pipe(pd)<0){
		perror("pipe()");
		exit(1);
	}

	pid = fork();
	if(pid<0){
		perror("fork()");
		exit(1);
	}
	if(pid==0){
		close(sd);
		close(pd[1]);
		dup2(pd[0],0);
		if(pd[0]>0){
			close(pd[0]);
		}
		pause();
		execl("/bin/sh","sh","-c", client_conf.player_cmd, NULL);
		exit(0);
	}
	fprintf(stderr, "children forked\n");
	close(pd[0]);

	msg_list = (msg_list_st*)malloc(MSG_LIST_MAX);
	if(msg_list==NULL){
		perror("malloc()");
		exit(1);
	}

	serveraddr_len = sizeof(serveraddr);
	while(1){
		len = recvfrom(sd, msg_list, MSG_LIST_MAX, 0, (struct sockaddr*)&serveraddr, &serveraddr_len);
		fprintf(stderr, "msg recieved.\n");
		if(len<sizeof(struct msg_list_st)){
			fprintf(stderr, "Message is too small.\n");
			continue;
		}
		if(msg_list->id != LISTCHNID){
			fprintf(stderr, "Chnid is not match.\n");
			continue;
		}
		break;
	}

	struct msg_listentry_st* pos;
	for(pos = msg_list->entry;(char*)pos<(((char*)(msg_list))+len);pos=(msg_listentry_st*)((char*)((char*)pos)+ntohs(pos->len))){
		printf("Channel %d: %s\n", pos->id, pos->descr);
	}
	do{
		ret = scanf("%d", &chosenid);
	}while(ret<1);

	//chosenid = 5;
	free(msg_list);

	msg_channel = (msg_channel_st*)malloc(MSG_CHANNEL_MAX);
        if(msg_channel==NULL){
                perror("malloc()");
                exit(1);
        }
	count=0;
	/* select channel */
	while(1){
		len = recvfrom(sd, msg_channel, MSG_CHANNEL_MAX, 0, (struct sockaddr*)&raddr, &raddr_len);
		if(raddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr || raddr.sin_port != serveraddr.sin_port){
			continue;
		}
		if(len<sizeof(struct msg_channel_st)){
			continue;
		}
		if(msg_channel->id == chosenid){
			/* play*/
			//writen(1, (char*)msg_channel->data, len - sizeof(struct msg_channel_st)-1);
			ret = writen(pd[1], (char*)msg_channel->data, len - sizeof(chnid_t));
			count += ret;
		}
		if(child_pause && count > 30000){
			kill(pid, SIGUSR1);
			child_pause = 0;
		}
	}
	free(msg_channel);
	close(sd);
	exit(0);
	return 0;
}
