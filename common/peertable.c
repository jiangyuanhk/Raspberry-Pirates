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



void tracker_initPeerTable(){
	tracker_peertable_headPtr = NULL;
	tracker_peertable_tailPtr = NULL;
	tracker_peertable_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(tracker_peertable_mutex, NULL);
}

void peer_initPeerTable(){
	peer_peertable_headPtr = NULL;
	peer_peertable_tailPtr = NULL;
	peer_peertable_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(peer_peertable_mutex, NULL);
}





void updatePeerTable(tracker_peer_t* entryPtr){
	char* ip = entryPtr->ip;

	//search table to find the entry with same ip
	pthread_mutex_lock(tracker_peertable_mutex);
	tracker_peer_t* iter = tracker_peertable_headPtr;
	while(iter != NULL){
		if(strcmp(iter->ip, ip) == 0)
			break;
		iter = iter->next;
	}
	pthread_mutex_unlock(tracker_peertable_mutex);

	//if not found, append the entry in the end of the table
	//if found, update found enty with this passing entry
	if(iter == NULL){
		appendPeerTable(entryPtr);
	} else {
		pthread_mutex_lock(tracker_peertable_mutex);
		//ip are the same so dont need to change its value
		iter->last_time_stamp = entryPtr->last_time_stamp; //time need to be updated
		iter->sockfd = entryPtr->sockfd; // TODO: are they always same ?
		pthread_mutex_unlock(tracker_peertable_mutex);
	}

}

void appendPeerTable(tracker_peer_t* newEntry){
	pthread_mutex_lock(tracker_peertable_mutex);
	if(tracker_peertable_headPtr == NULL){
		tracker_peertable_headPtr = newEntry;
		tracker_peertable_tailPtr = newEntry;
	}else{
		tracker_peertable_tailPtr->next = newEntry;
		tracker_peertable_tailPtr = newEntry;
	}
	pthread_mutex_unlock(tracker_peertable_mutex);
}











