/* File: filetable.c
   Description: Describes functions associated with creating and manipulating the filetable
   		used by both the peer and tracker.  Unit tested in the testing directory with 
   		filetable_test.c
*/

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
#include <time.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>

#include "filetable.h"
#include "peertable.h"

/* Function to initialize a file table.  The head and tail of the filetable will be set to 
	NULL and the size is 0.  A mutex lock is malloced for the filetable.

	@return the pointer to the fileTable_t that is created.
*/
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

/*
  Function to initialize a file entry.
*/
fileEntry_t * filetable_createFileEntry(char* filepath, int size, unsigned long int timestamp, int type){
  fileEntry_t* entry = malloc(sizeof(fileEntry_t));
  entry -> peerNum = 0;
  entry -> file_type = type;
  memset(entry -> file_name, 0, FILE_NAME_MAX_LEN);
  memcpy(entry -> file_name, filepath, strlen(filepath) + 1);
  entry -> size = size;
  entry -> timestamp = timestamp;

  return entry;
}

/**
 * Iterates dest table to check if there exist an fileEntry with filename
 * Used the mutex lock for the table.
 * @param  tablePtr      [head of fileEntries in dest fileTable]
 * @param  filename  [filename]
 * @return           [pointer to entry if found, NULL if cannot find]
 */
fileEntry_t* filetable_searchFileByName(fileTable_t* tablePtr, char* filename) {

  if(tablePtr -> size == 0) return NULL;

  pthread_mutex_lock(tablePtr -> filetable_mutex);
  
  fileEntry_t* iter = tablePtr -> head;
  while(iter != NULL) {

    //if the entry matches the name, the file is in the table so return that entry
		if(strcmp(iter -> file_name, filename) == 0){
      pthread_mutex_unlock(tablePtr -> filetable_mutex);
			return iter;
		}

    //otherwise, move to the next file in the table
		iter = iter -> next;
	}

  pthread_mutex_unlock(tablePtr -> filetable_mutex);
	return NULL;
}

/**
 * Iterates through a file table to check if there exist an fileEntry with filename
 * Does not take advantage of a mutex lock.
 * NOTE: Try to use previous function if possible.
 * @param  head      [head of fileEntries in dest fileTable]
 * @param  filename  [filename]
 * @return           [pointer to entry if found, NULL if cannot find]
 */
fileEntry_t* filetable_searchFileByNameWithoutMutex(fileEntry_t* head, char* filename) {
  fileEntry_t* iter = head;

  while(iter){
    if(strcmp(iter->file_name, filename) == 0){
      return iter;
    }
    iter = iter -> next;
  }
  return NULL;

}

/**
 * Check table for file and delete if the file is present.
 * @param  tablePtr  [pointer to the fileTable]
 * @param  filename  [filename]
 * @return           [1 if successfully deleted, -1 if could not be deleted]
 */
int filetable_deleteFileEntryByName(fileTable_t* tablePtr, char* filename){

	assert(tablePtr != NULL);
	assert(filename != NULL);

	if(tablePtr -> size == 0) return -1; //table is zero-size

	pthread_mutex_lock(tablePtr->filetable_mutex);
	fileEntry_t* file = tablePtr -> head;
	
	//check if table head needs to be replaced
	if(strcmp(file -> file_name, filename) == 0) {
		
		//if the head is the only file, updated the head and tail pointers to be NULL
		if (tablePtr -> size == 1) {
			tablePtr -> head = NULL;
			tablePtr -> tail = NULL;
		}

		//otherwise update the head to be the file after the head
		else {
			tablePtr -> head = tablePtr -> head -> next;
		}

    tablePtr -> size -= 1;
		
    free(file);
    pthread_mutex_unlock(tablePtr->filetable_mutex);

		return 1;
	}

  //otherwise, the file is either in the list and not the head or is not in the list
  else {
    fileEntry_t* prev_file = file;
    file = file -> next;

    //looping until reach the end of the list or find the file
    while(file != NULL){

      //if find the file in the list, remove it and update pointers
      if (strcmp(file -> file_name, filename) == 0) {
        prev_file -> next = file -> next;

        //if the file is the tail, update the tail to be the previous file in the list
        if (file == tablePtr -> tail) {
          tablePtr -> tail = prev_file;
        }

        tablePtr -> size -= 1;

        free(file);
        pthread_mutex_unlock(tablePtr -> filetable_mutex);

        return 1;
      }

      //move the pointers to the next pair of files.
      prev_file = file;
      file = file -> next;
    }

    //if reach here, then the file is not found and return -1
    pthread_mutex_unlock(tablePtr -> filetable_mutex);
    return -1;

  }
}


/**
 * Adds a new file entry to the end of the fileEntryTable.
 * @param  tablePtr     [pointer to the fileTable]
 * @param  newEntryPtr  [pointer to the file entry to add]
 */
void filetable_appendFileEntry(fileTable_t* tablePtr, fileEntry_t* newEntryPtr){
	assert(tablePtr != NULL && newEntryPtr != NULL);

	pthread_mutex_lock(tablePtr -> filetable_mutex);

  //if the table is empty, set the new file entry to be the head and tail
  if (tablePtr -> size == 0) {
		tablePtr -> head = newEntryPtr;
    tablePtr -> tail = newEntryPtr;
	} 

  //otherwise, append the new file entry to the end of the list and make the new file the tail
  else {
		tablePtr -> tail -> next = newEntryPtr;
		tablePtr -> tail = newEntryPtr;
	}

	tablePtr -> size ++;

	pthread_mutex_unlock(tablePtr -> filetable_mutex);
	return;
}

/**
 * Prints out a file table
 * @param  tablePtr     [pointer to the fileTable]
 */
void filetable_printFileTable(fileTable_t* tablePtr) {
	fileEntry_t* iter = tablePtr->head;
	printf("FILETABLE: \n");
	printf("==========================\n");
	while(iter != NULL) {
		//print out information
		time_t rawtime;
		rawtime = iter->timestamp;
		
		printf("--filename: %s \ttype: %d\tlastModifiedTime: %s\tfileSize: %u \tpeerIPs: ", 
			iter -> file_name, iter -> file_type, ctime(&rawtime), iter->size);
		
		int i = 0;
		while (i < iter->peerNum) {
			printf("%s \n", iter->iplist[i]);
      i++;
		}

		printf("\n");
		iter = iter-> next;
	}
	printf("=========================\n");

}


/**
 * Update oldEntry's timestamp and size by replacing these values with those
 * of the newEntry. Only do so if the two entries have the same name.
 * make sure two entries have the same name
 * @param  oldEntryPtr [the old file entry whose values will be updated (replaced)]
 * @param  newEntryPtr [the new file entry whose values will be used to update the old file entry]
 * @param  tablemutex  [the mutex lock to lock the file table while updating the filetable]
 * @return             [returns 1 if the filetable was successfully, -1 otherwise]
 */
int filetable_updateFile(fileEntry_t* oldEntryPtr, fileEntry_t* newEntryPtr, pthread_mutex_t* tablemutex) {
	
  //if the file names do not match, do not update the files and return -1
	if(strcmp(oldEntryPtr->file_name, newEntryPtr->file_name)!= 0)
		return -1;

  //otherwise, update the old entry to reflect the values of the new entry
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
int filetable_AddIp2Iplist(fileEntry_t* entry, char* peerip, pthread_mutex_t* tablemutex) {
	
	pthread_mutex_lock(tablemutex);
	int i = 0;

  //loop through the list and see if the ip address is already in the list
	for(; i < entry -> peerNum; i++){
		
    //if they match, it is in the list
    if(strcmp(entry->iplist[i], peerip) == 0){
			pthread_mutex_unlock(tablemutex);
			return -1;
		}
	}

  //otherwise the ip did not exist before, so if there is additional space 
  //in the ip list, add the ip to the end of the list
	if(i < MAX_NUM_PEERS){
		
		strcpy(entry -> iplist[i], peerip);
		entry -> peerNum++;

		pthread_mutex_unlock(tablemutex);
		return 1;
	}

	//otherwise, the ipnot does not exist but iplist is full so it could not be added
	pthread_mutex_unlock(tablemutex);
	return -1;
}

/**
 * Delete a peer ip from the ip list of the given entry
 * @param  entry  [file entry you want to remove the ip from]
 * @param  peerip [the ip you want to remove from the list]
 * @param  mutex  [the mutex for the file table the entry is from to avoid errors]
 * @return        [return 1 if deletion happens, -1 if no deletion]
 */
int filetable_deleteIpfromIplist(fileEntry_t* entry, char* peerip, pthread_mutex_t* tablemutex) {

	pthread_mutex_lock(tablemutex);
	int i;

  //loop through all of the peers in the peer ip list
	for(i = 0; i < entry->peerNum; i++) {

    //if the peer ip matches the entry in the peer, remove it
		if(strcmp(peerip, entry->iplist[i]) == 0){
			
			// copy the last ip of the iplist into the locaiton and decrement the peer num
      // since the last ip has moved to the earlier location or has been deleted
			memcpy(entry -> iplist[i], entry -> iplist[entry->peerNum - 1], IP_LEN);
      memset(entry -> iplist[entry->peerNum - 1], 0, IP_LEN);

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
		//loop over all entries in the ip list for the file entry
		for(i = 0; i < iter->peerNum; i++){
			
      //if the ip entry matches, remove it from the list
      if(strcmp(peerip, iter->iplist[i]) == 0){

        memcpy(iter->iplist[i], iter -> iplist[iter->peerNum - 1], IP_LEN);
        memset(iter -> iplist[iter->peerNum - 1], 0, IP_LEN);
        
				iter->peerNum --;
				deleteHappens = 1;
			}
		}
    iter = iter -> next;
	}

	pthread_mutex_unlock(table->filetable_mutex);
	return deleteHappens;
}

/**
 * Destroys the filetable by freeing each entry in the filetable,
 * the mutex lock, and the table itself.
 * @param tablePtr [file table pointer]
 */
void filetable_destroy(fileTable_t *tablePtr){

  //if the file table size is not zero, free the file entries
	if(tablePtr->size != 0){
		
    pthread_mutex_lock(tablePtr->filetable_mutex);
		
    //loop through each file entry in the table freeing them
    fileEntry_t* iter = tablePtr->head;
		while(iter){
			fileEntry_t* prev = iter;
			iter = iter -> next;
			free(prev);
		}
		pthread_mutex_unlock(tablePtr -> filetable_mutex);
	}

	free(tablePtr->filetable_mutex);
  free(tablePtr);
	return;
}	

/******************** ARRAY <==========> LINKEDLIST CONVERSION ******************/

/**
 * conver linkedlist of entries of in the table into continous chunk of array, for ease of sending
 * need to pass tablePtr because we want the tablesize and head/tail information together
 * @param  entry 	[head pointer of linked list of fileEntries in the table]
 * @num             [number of file entries in the linked list]
 * @tablemutex      [the mutex lock for the filetable]
 * @return          [array chunk]
 */
char* filetable_convertFileEntriesToArray(fileEntry_t* entry, int num, pthread_mutex_t* tablemutex){
	
	pthread_mutex_lock(tablemutex);
	//initialize the array
	char* buf = malloc(num * sizeof(fileEntry_t));

  memset(buf, 0, num * sizeof(fileEntry_t));
	fileEntry_t* iter = entry;
	int i = 0;
	while(iter != NULL){
		//each time, copy size of fileEntry_t from iter to the array indentified by arrayHead
		memcpy(&buf[i * sizeof(fileEntry_t)], iter, sizeof(fileEntry_t));
		//go to next entry
		iter = iter-> next;
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

  if (num < 1) return NULL;

	//create and read in the head of the list first
	fileEntry_t* head = (fileEntry_t*) malloc(sizeof(fileEntry_t));
  memcpy(head, buf, sizeof(fileEntry_t));
  head -> next = NULL;

  //read in the rest of the entries, adding them to the SLL with the head at the start
  fileEntry_t* prev = head;
  fileEntry_t* curr;

	int i;
	for(i = 1; i < num; i++) {
		curr = malloc(sizeof(fileEntry_t));
		memcpy(curr, buf + i * sizeof(fileEntry_t), sizeof(fileEntry_t));
    curr -> next = NULL;
    prev -> next = curr;
    prev = curr;
	}

	return head;
}

/**
 * Convert entries into a file table.
 * NULL if buffer is empty
 * @param  head_file [the first file in a linked list of files to convert into a file table]
 * @return     [filetable pointer]
 */
fileTable_t* filetable_convertEntriesToFileTable(fileEntry_t* head_file) {

  fileTable_t* filetable = filetable_init();
  
  fileEntry_t* entry = head_file;

  //loop through each entry and add it to the table
  while (entry != NULL) {
    filetable_appendFileEntry(filetable, entry);
    entry = entry -> next;
  }

  //make sure the tail has a next of NULL if it exists
  if (filetable -> tail){
  	filetable -> tail -> next = NULL;
  }

	return filetable;
}








