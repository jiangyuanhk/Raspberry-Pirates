

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "downloadFileList.h"
#include "../common/constants.h"
#include <pthread.h>


//TODO: added mutex lock and unlock to all the operations

DLL_t* DLL_initList(){

	DLL_t* list = (DLL_t*)malloc(sizeof(DLL_t));
	list->head = NULL;
	list->tail = NULL;
	list -> size = 0;
	//create the mutex for the table
	pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
	list->mutex = mutex;

}



DLLEntry_t*	DLL_createEntry(char* filename){
	DLLEntry_t* entry = (DLLEntry_t*) malloc(sizeof(DLLEntry_t));
	entry->next = NULL;
	memcpy(entry->filename, filename, FILE_NAME_MAX_LEN); //TODO: memcpy or strcpy ? 
	return entry;
}


/**
 * add new file to the download list
 * return 1 if successfully added, return -1 if has file with the same filename already
 * 
 */
int DLL_addEntry(DLL_t* list, char* filename){
	
	//go through the list to see if entry exist
	pthread_mutex_lock(list->mutex);
	DLLEntry_t* iter = list->head;
	while(iter != NULL){
		if(strcmp(iter->filename, filename) == 0)
			pthread_mutex_unlock(list->mutex);
			return -1;
		iter = iter->next;
	}
	//now, no file has the same filename
	DLLEntry_t* head = DLL_createEntry(filename);
	if(list->size == 0) {
		//if empty create a head 
		list->head = head;
		list->tail = head;
		list->size = 1;

	} else {
		//not empty
		list->size++;
		list->tail->next = head;
		list->tail = list->tail->next;
	}
	pthread_mutex_unlock(list->mutex);
	return 1;
}


int DLL_removeEntry(DLL_t* list, char* filename){
	if(list->size == 0)
		return -1;

	DLLEntry_t dummy;
	dummy.next = list->head;

	DLLEntry_t* iter = &dummy;

	while(iter->next != NULL){

		if(strcmp(iter->next->filename, filename) == 0){
			DLLEntry_t* tobeDeleted = iter->next;
			iter->next = iter->next->next;
			free(tobeDeleted);
			return 1;
		}
		iter = iter->next;
	}

	return -1;

}

int DLL_destroy(DLL_t* list){

	if(list->size > 0){
		DLLEntry_t* iter = list->head;
		while(iter){
			DLLEntry_t* tobeDeleted = iter;
			iter = iter->next;
			free(tobeDeleted);
		}
	}
	free(list->mutex);
	free(list);
	return 1;
}