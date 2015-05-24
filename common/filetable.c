

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
#include <time.h>
#include <assert.h>

fileTable_t* filetable_init() {

	fileTable_t* tablePtr = (fileTable_t*) malloc(sizeof(fileTable_t));
	tablePtr->head = NULL;
	tablePtr->tail = NULL;
	tablePtr->size = 0;
	
	//create the mutex for the table
	pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
	tablePtr->filetable_mutex = mutex;

	return tablePtr;

}




fileEntry_t* filetable_searchFileByName(fileTable_t* tablePtr, char* filename){
	fileEntry_t* iter = tablePtr->head;


	while(iter){
		if(strcmp(iter->name, filename) == 0){
			return iter;
		}
		iter = iter->pNext;
	}

	return NULL;

}




int filetable_deleteFileEntryByName(fileTable_t* tablePtr, char* filename){

	assert(tablePtr != NULL);
	assert(filename != NULL);


	if(tablePtr->size == 0) return -1; //table is zero-size
	pthread_mutex_lock(tablePtr->filetable_mutex);

	fileEntry_t* dummy = (fileEntry_t*) malloc(sizeof(fileEntry_t));
	dummy->pNext = tablePtr->head;

	fileEntry_t* iter = dummy; // dummy head
	while(iter->pNext){
		if(strcmp(iter->pNext->name, filename) == 0){
			//found, delete iter->pNext
			fileEntry_t* temp = iter->pNext;  // to be deleted
			iter->pNext = iter->pNext->pNext;
			tablePtr->size -= 1;
			free(temp);
			free(dummy);
			pthread_mutex_unlock(tablePtr->filetable_mutex);
			return 1;
		}

		iter = iter->pNext;
	}
	free(dummy);
	pthread_mutex_unlock(tablePtr->filetable_mutex);
	return -1; // cannot find the file in the table


}



void filetable_appendFileEntry(fileTable_t* tablePtr, fileEntry_t* newEntryPtr){
	assert(tablePtr != NULL && newEntryPtr != NULL);

	pthread_mutex_lock(tablePtr->filetable_mutex);
	if(tablePtr->size == 0){
		tablePtr->head = tablePtr->tail = newEntryPtr;
	} else {
		tablePtr->tail = newEntryPtr;
		tablePtr->tail = tablePtr->tail->pNext;
	}

	tablePtr->size ++;
	pthread_mutex_unlock(tablePtr->filetable_mutex);
	return;

}



void filetable_printFileTable(fileTable_t* tablePtr){
	fileEntry_t* iter = tablePtr->head;
	printf("FILETABLE: \n");
	printf("==========================\n");
	while(iter != NULL) {
		//print out information
		time_t rawtime;
		rawtime = iter->timestamp;
		
		printf("--filename: %s \tlastModifiedTime: %s\tfileSize: %u \tpeerIPs: ", 
			iter->name, ctime(&rawtime), iter->size);
		
		int i = 0;
		while (i < iter->peerNum) {
			printf("%s \n", iter->iplist[0]);
		}

		printf("\n");
		iter = iter->pNext;
	}
	printf("=========================\n");

}


/**
 * use the old entry to update newEntry in the fietable
 */

int filetable_updateFile(fileTable_t* tablePtr, fileEntry_t* oldEntryPtr, fileEntry_t* newEntryPtr){
	
	if(strcmp(oldEntryPtr->name, newEntryPtr->name)!= 0)
		return -1;
	pthread_mutex_lock(tablePtr->filetable_mutex);

	memcpy(&(oldEntryPtr->size), &(newEntryPtr->size), sizeof(int));
	memcpy(&(oldEntryPtr->timestamp), &(newEntryPtr->timestamp), sizeof(unsigned long int));

	pthread_mutex_unlock(tablePtr->filetable_mutex);
	return 1;
}




void fileTable_destroy(fileTable_t *tablePtr){

	if(tablePtr->size != 0){
		pthread_mutex_lock(tablePtr->filetable_mutex);
		fileEntry_t* iter = tablePtr->head;
		while(iter){
			fileEntry_t* cur = iter;
			iter = iter->pNext;
			free(cur);
		}
		pthread_mutex_unlock(tablePtr->filetable_mutex);
	}

	free(tablePtr->filetable_mutex);
	return;
}	












