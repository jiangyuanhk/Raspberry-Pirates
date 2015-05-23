

#include "filetable.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "peertable.h"
#include "peertable."




/**
 * [search the file (type: Node) in the filetable by filename]
 * @param  filename [description]
 * @return          [Null if ]
 */
Node* searchFileByName(char*  filename){
 	pthread_mutex_lock(file_table_mutex);
	Node * itr = filetable_headPtr;
	while(itr){
		if(strcmp(filename, itr -> name) == 0){
 			pthread_mutex_unlock(file_table_mutex);
			return itr;
		}
		itr = itr->pNext;
	}
 	pthread_mutex_unlock(file_table_mutex);
	return itr;
}	




int countFiles(){

    if(filetable_headPtr == NULL) return 0;
    int cnt = 0;
    Node* iter = filetable_headPtr;
    while(iter != filetable_tailPtr){
        cnt++;
        iter  = iter->pNext;
    }

    return cnt + 1;
}








