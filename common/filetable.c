

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



/**
 * iterate dest table to check if there exist an fileEntry with filename
 * @param  head      [head of fileEntries in dest fileTable]
 * @param  filename  [filename]
 * @return           [pointer to entry if found, NULL if cannot find]
 */
fileEntry_t* filetable_searchFileByName(fileEntry_t* head, char* filename){
	fileEntry_t* iter = head;


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
 * update oldEntry's timestamp, size using newEntry's infromation
 * make sure two entries have the same name
 * @param  tablePtr    [description]
 * @param  oldEntryPtr [description]
 * @param  newEntryPtr [description]
 * @return             [description]
 */
int filetable_updateFile(fileEntry_t* oldEntryPtr, fileEntry_t* newEntryPtr, pthread_mutex_t* tablemutex){
	
	if(strcmp(oldEntryPtr->name, newEntryPtr->name)!= 0)
		return -1;
	pthread_mutex_lock(tablemutex);
	memcpy(&(oldEntryPtr->size), &(newEntryPtr->size), sizeof(int));
	memcpy(&(oldEntryPtr->timestamp), &(newEntryPtr->timestamp), sizeof(unsigned long int));
	pthread_mutex_unlock(tablemutex);
	return 1;
}




/**
 * insert an new peerip into fileEntry's iplist if this is a new peerip, and return 1
 * return -1 if the peerip already exist in the fileEntry's iplist
 * @param  entry [fileEntry whose iplist to be searched and added if needed]		
 * @param  peerip    [peerip to be insert if possible]
 * @param  mutex [mutex of the fileTable to which entry belong]
 * @return       [1 if insert success, -1 if peerip already exist]
 */
int filetable_AddIp2Iplist(fileEntry_t* entry, char* peerip, pthread_mutex_t* tablemutex){
	

	pthread_mutex_lock(tablemutex);
	int i = 0;
	for(; i < entry->peerNum; i++){
		if(strcmp(entry->iplist[i], peerip) == 0){
			//already exist
			pthread_mutex_unlock(tablemutex);
			return -1;
		}
	}

	if(i < MAX_PEER_NUM){
		//not exist before, and iplist has additional space
		strcpy(entry->iplist[i], peerip);
		entry->peerNum++;

		pthread_mutex_unlock(tablemutex);
		return 1;
	}

	//not exist, but iplist is full
	pthread_mutex_unlock(tablemutex);
	return -1;

}


/**
 * delete peerip from one entry's iplist, if any
 * @param  entry  [description]
 * @param  peerip [description]
 * @param  mutex  [description]
 * @return        [return 1 if deletion happens, -1 if no deletion]
 */
int filetable_deleteIpfromIplist(fileEntry_t* entry, char* peerip, pthread_mutex_t* tablemutex){

	pthread_mutex_lock(tablemutex);
	int i;
	for(i = 0; i < entry->peerNum; i++){
		//loop through all peers
		if(strcmp(peerip, entry->iplist[i]) == 0){
			//exist, then delete (at most appear once)
			//swap currnet ip with the last ip of the iplist 
			//no need to care about last ip then, only need to decrement peerNum --
			strcpy(entry->iplist[i], entry->iplist[entry->peerNum - 1]); 
			entry->peerNum --;
			pthread_mutex_unlock(tablemutex);
			return 1;
		}
	}
	pthread_mutex_unlock(tablemutex);
	return -1;
}

	
/**
 * dont call deleteIpfromIplist because mutex would be called mutiple times
 * loop all entries in the table, delete the peerip if any entry has it
 * @param  table  [description]
 * @param  peerip [description]
 * @return        [return 1 if deletion happens, -1 if no deletion]
 */
int filetable_deleteIpfromAllEntries(fileTable_t* table, char* peerip){
	if(table->size == 0) return -1;
	pthread_mutex_lock(table->filetable_mutex);
	fileEntry_t* iter = table->head;
	int i;
	int deleteHappens = -1;
	while(iter != NULL){
		//loop over all entries
		for(i = 0; i < iter->peerNum; i++){
			if(strcmp(peerip, iter->iplist[i]) == 0){
				//exist, then swap last ip with current ip
				//decrement peerNum by 1
				strcpy(iter->iplist[i], iter->iplist[iter->peerNum - 1]);
				iter->peerNum --;
				deleteHappens = 1;
			}
		}
	}

	pthread_mutex_unlock(table->filetable_mutex);
	return deleteHappens;
}





/**
 * destroy the whole filetable
 * @param tablePtr [table pointer]
 */
void filetable_destroy(fileTable_t *tablePtr){

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




/******************** ARRAY <==========> LINKEDLIST CONVERSION ******************/

/**
 * conver linkedlist of entries of in the table into continous chunk of array, for ease of sending
 * need to pass tablePtr because we want the tablesize and head/tail information together
 * @param  entry 	[head pointer of linked list of fileEntries in the table]
 * @return          [array chunk]
 */
char* filetable_convertFileEntriesToArray(fileEntry_t* entry, int num, pthread_mutex_t* tablemutex){
	
	pthread_mutex_lock(tablemutex);
	//initialize the array
	char* buf = (char*) malloc(num * sizeof(fileEntry_t));

	fileEntry_t* iter = entry;
	int i = 0;
	while(iter != NULL){
		//each time, copy size of fileEntry_t from iter to the array indentified by arrayHead
		memcpy(buf + i * sizeof(fileEntry_t), iter, sizeof(fileEntry_t));
		//go to next entry
		iter = iter->pNext;
		i++;
	}
	pthread_mutex_unlock(tablemutex);
	return buf;
}





/**
 * convert received buffer back to a list of fileEntries struct
 * NULL if buffer is empty
 * @param  buf [received buffer (containing linkedlist of entries)]
 * @param  num [size of the table]
 * @return     [filetable pointer]
 */
fileEntry_t* filetable_convertArrayToFileEntires(char* buf, int num){


	//create a dummy head first
	fileEntry_t* dummy = (fileEntry_t*) malloc(sizeof(fileEntry_t));
	dummy->pNext = NULL;

	//iterator points to dummy at first
	fileEntry_t* iter = dummy;

	//create entries one by one
	int i;
	for(i = 0; i < num; i++){
		//create an entry
		fileEntry_t* entry = (fileEntry_t*) malloc(sizeof(fileEntry_t));
		memcpy(entry, buf + i * sizeof(fileEntry_t), sizeof(fileEntry_t));
		entry->pNext = NULL;
		iter->pNext = entry;
		iter = entry->pNext;
	}

	return dummy->pNext;
}








