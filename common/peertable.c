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



peerTable_t* peerTable_init(){
	peerTable_t *peertable = (peerTable_t*) malloc(sizeof(peerTable_t));
	peertable->head = NULL;
	peertable->tail = NULL;
	peertable->size = 0;
	return peertable;
}


int peerTable_addEntry(peerTable_t *table, char* ip, int sockfd) {

	peerEntry_t* tableEntry;

    // Allocate memory for the entry.
    tableEntry = (peerEntry_t*)malloc(sizeof(peerEntry_t));

    // Set initial fields for the entry.
    memcpy(tableEntry->ip, ip, IP_LEN);
    tableEntry->sockfd = sockfd;
    tableEntry->timestamp = time(NULL);
    tableEntry->next = NULL;

    // when table is empty.
    if (table->size == 0) {
    	table->head = tableEntry;
    	table->tail = tableEntry;
    	table->size = table->size + 1;
        return 1;
    }


    // when not empty, append to last
    table->tail->next = tableEntry;
    table->tail = table->tail->next;

   	return 1;
}



// This method removes a table entry given the IP of the node to delete. Also fixes next pointers.
// Returns 1 on success, -1 on failure.
int peerTable_deleteEntryByIp(peerTable_t *table, char* ip) {


	if(table->size == 0) return -1; //empty table

	pthread_mutex_lock(table->peertable_mutex);
	peerEntry_t* dummy = (peerEntry_t*)malloc(sizeof(peerEntry_t));
	dummy->next = table->head;
	peerEntry_t* iter = dummy;

	while(iter->next != NULL){
		if(strcmp(iter->next->ip, ip) == 0){

			peerEntry_t* temp = iter->next;
			iter->next = iter->next->next;
			free(temp);
			free(dummy);
			table->size -= 1;
			pthread_mutex_unlock(table->peertable_mutex);
			return 1;
		}
		iter = iter->next;
	}

	free(dummy);
	pthread_mutex_unlock(table->peertable_mutex);
	return -1; // not found

}




void peerTable_destroy(peerTable_t *table) {
	if(table->size != 0){
		pthread_mutex_lock(table->peertable_mutex);
		peerEntry_t* iter = table->head;
		while(iter){
			peerEntry_t* cur = iter;
			iter = iter->next;
			free(cur);
		}
		pthread_mutex_unlock(table->peertable_mutex);
	}

	free(table->peertable_mutex);
	return;
}













