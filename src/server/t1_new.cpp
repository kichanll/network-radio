#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

#include "medialib.h"


int main(int argc, char* argv[]){
	int err, i;
	struct mlib_listentry_st* list;
	int list_size;

	err = mlib_getchnlist(&list, &list_size);
        if(err){
                syslog(LOG_ERR, "mlib_getchnlist() %s", strerror(err));
                exit(1);
        }

	for(i=0;i<list_size;++i){
		printf("CHN:%d %s\n", list[i].id, list[i].desc);
	}
	return 0;
}
