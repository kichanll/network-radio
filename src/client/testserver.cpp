#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>

#include <proto.h>

int main(int argc, char* argv[]){
	int sd, i;
	struct msg_list_st* listbuf;
	struct msg_listentry_st* tmp;
	struct ip_mreqn mreq;
	struct sockaddr_in raddr;
	int id[3] = {12, 23, 34};
	char *descr[3] = {"Music", "Opera", "Talks"};

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd<0){
		perror("socket()");
		exit(1);
	}

	inet_pton(AF_INET, "224.2.2.2", &mreq.imr_multiaddr);
        inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
        mreq.imr_ifindex = if_nametoindex("eth0");
        if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq))<0){
                perror("setsockopt()");
                exit(1);
        }

	listbuf = (msg_list_st*)malloc(28);
	if(listbuf == NULL){
		perror("malloc()");
		exit(1);
	}

	listbuf->id = LISTCHNID;
	tmp = listbuf->entry;
	for(i = 0;  i < 3; ++i){
		tmp->id = id[i];
		tmp->len = htons(9);
		strcpy((char*)tmp->descr, descr[i]);
		tmp = (msg_listentry_st*)(((char*)tmp)+9);
	}

	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(1999);
	inet_pton(AF_INET, "224.2.2.2", &raddr.sin_addr);
	while(1){
		sendto(sd, listbuf, 28, 0, (struct sockaddr*)&raddr, sizeof(raddr));
		sleep(1);
	}
}
