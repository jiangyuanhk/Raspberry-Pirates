#include "peertable.h"
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
#include "../common/utils.h"


/* Function to initialize a peer table.  The head and tail of the peer table will be set to 
	NULL and the size is 0.

	@return the pointer to the peerTable_t* that is created.
*/
peerTable_t* peertable_init(){
	peerTable_t *peertable = (peerTable_t*) malloc(sizeof(peerTable_t));
	peertable -> head = NULL;
	peertable -> tail = NULL;
	peertable -> size = 0;

  //create the mutex for the table
  pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  peertable -> peertable_mutex = mutex;

	return peertable;
}

/**
 * Create a peertable entry from the given sockfd and ip.
 * @param  head      [head of fileEntries in dest fileTable]
 * @param  filename  [filename]
 * @return           [pointer to entry if found, NULL if cannot find]
 */
peerEntry_t* peertable_createEntry(char* ip, int sockfd){
	// Allocate memory for the entry.
  peerEntry_t* peerEntry = (peerEntry_t*)malloc(sizeof(peerEntry_t));

  // Set initial fields for the entry.
  memcpy(peerEntry->ip, ip, IP_LEN);
  peerEntry -> sockfd = sockfd;
  peerEntry -> timestamp = getCurrentTime();
  peerEntry -> next = NULL;

	return peerEntry;
}

/**
 * iterate peer table to check if there exist an peerEntry with the same ip
 * @param  table     [peertable to check if the entry exists in]
 * @param  ip        [ip we are checking if it is in the table]
 * @return           [the peerentry_t if the ip is found, NULL otherwise]
 */
peerEntry_t* peertable_searchEntryByIp(peerTable_t* table, char* ip) {
  
  if(table->size == 0) return NULL;

  pthread_mutex_lock(table->peertable_mutex);
  
  peerEntry_t* iter = table->head;
  while(iter != NULL) {
    
    //if the entry matches the ip, the ip has a peer entry so return it
    if (strcmp(iter -> ip, ip) == 0) {
      pthread_mutex_unlock(table->peertable_mutex);
      return iter;
    }
    
    iter = iter->next;
  }

  pthread_mutex_unlock(table->peertable_mutex);
  return NULL;
}

/**
 * Adds a new peer entry to the end of the peer Table.
 * @param  table  [pointer to the peer Table]
 * @param  entry  [pointer to the peer entry to add]
 */
int peertable_addEntry(peerTable_t* table, peerEntry_t* entry) {

  pthread_mutex_lock(table -> peertable_mutex);
  // when table is empty, add the entry and make it the head and tail
  if (table -> size == 0) {
  	table -> head = entry;
  	table -> tail = entry;
  }

  // otherwise, append the new entry after the tail and make it the new tail
  else {
    table -> tail -> next = entry;
    table -> tail = entry;
  }

  table -> size ++;

  pthread_mutex_unlock(table -> peertable_mutex);
 	return 1;
}

/**
 * Removes a table entry given the IP addressof the node to delete.
 * Also, updates the necessary pointers upon deletion.
 * @param  table  [pointer to the peer Table]
 * @param  ip     [pointer to ip address to delete]
 * @return [returns 1 on success, -1 on failure]
 */
int peertable_deleteEntryByIp(peerTable_t *table, char* ip) {

	if(table -> size == 0) return -1; //table is zero-size

  pthread_mutex_lock(table -> peertable_mutex);
  peerEntry_t* file = table -> head;
  
  //check if table head needs to be replaced
  if(strcmp(file -> ip, ip) == 0) {
    
    //if the head is the only file, updated the head and tail pointers to be NULL
    if (table -> size == 1) {
      table -> head = NULL;
      table -> tail = NULL;
    }

    //otherwise update the head to be the file after the head
    else {
      table -> head = table -> head -> next;
    }

    table -> size -= 1;
    
    free(file);
    pthread_mutex_unlock(table->peertable_mutex);

    return 1;
  }

  //otherwise, the file is either in the list and not the head or is not in the list
  else {
    peerEntry_t* prev_file = file;
    file = file -> next;

    //looping until reach the end of the list or find the file
    while(file != NULL) {

      //if find the file in the list, remove it and update pointers
      if (strcmp(file -> ip, ip) == 0) {
        prev_file -> next = file -> next;

        //if the file is the tail, update the tail to be the previous file in the list
        if (file == table -> tail) {
          table -> tail = prev_file;
        }

        table -> size -= 1;

        free(file);
        pthread_mutex_unlock(table -> peertable_mutex);

        return 1;
      }

      //move the pointers to the next pair of files.
      prev_file = file;
      file = file -> next;
    }

    //if reach here, then the file is not found and return -1
    pthread_mutex_unlock(table -> peertable_mutex);
    return -1;
  }
}


/**
 * Destroys the table by dynamically freeing all of the entries
 * in the table and the table itself.
 * @param  table  [pointer to the peer table to be freed] 
 */
void peertable_destroy(peerTable_t *table) {
	
	// if there are entries in the table, free each of the entries
	if(table -> size != 0) {
		
    pthread_mutex_lock(table -> peertable_mutex);
		
    //loop through each peer, freeing as we go
    peerEntry_t* iter = table->head;
		while(iter){
			peerEntry_t* prev = iter;
			iter = iter -> next;
			free(prev);
		}
		pthread_mutex_unlock(table -> peertable_mutex);
	}

	//free table mutex and table itself
	free(table -> peertable_mutex);
  free(table);

	return;
}

/**
 * Prints out a peer table
 * @param  tablePtr     [pointer to the fileTable]
 */
void peertable_printPeerTable(peerTable_t* peertable) {
  peerEntry_t* iter = peertable->head;
  printf("PEERTABLE:        Size: %d\n", peertable -> size);
  printf("==========================\n");
  
  while(iter != NULL) {
    //print out information
    time_t rawtime;
    rawtime = iter -> timestamp;

    printf("--ip: %s\tTimestamp: %s sockfd: %d", 
      iter -> ip, ctime(&rawtime), iter->sockfd);
    
    printf("\n");
    iter = iter-> next;
  }
  printf("=========================\n");

}


/**
 * Refresh the peerEntry's timestamp to latest time
 * @param  entry [the entry whose timestamp will be updated]
 * @return       [1 if successful, -1 if fail]
 */
int peertable_refreshTimestamp(peerEntry_t* entry){
    
  unsigned long curTime = getCurrentTime();

  //if the entries time stamp is greater than the current time, return -1 
  if(entry -> timestamp > curTime) return -1;
  
  //otherwise update the entries time stamp to be the current time
  entry -> timestamp = curTime;
  return 1;
}









