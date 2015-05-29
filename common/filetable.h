#ifndef FILETABLE_H
#define FILETABLE_H

#include "constants.h"
#include <pthread.h>

#define FILE 0
#define DIRECTORY 1

/**
 * each file is represented as a fileEntry
 */
typedef struct fileEntry{

 int size;                          //the size of the file
 char file_name[FILE_NAME_MAX_LEN]; //the name of the file, must be unique in the same directory
 unsigned long int timestamp;       //the timestamp when the file is modified or created
 struct fileEntry* next;            //pointer to build the linked list
 char iplist[MAX_NUM_PEERS][IP_LEN]; //tracker:  this is a list of peers' ips posessing the file
                                    //peer:     only contains ip of peer itself, put it in iplist[0]
 int peerNum;   					//tracker: number of peers in the ip list                    
 int file_type;
}fileEntry_t;



/**
 * the file table are defined as a linked list of fileEntries
 * we keep track head, tail and the size of the linkedList
 */
typedef struct fileTable{
    fileEntry_t* head;  // header of file table
    fileEntry_t* tail; // tail of file table (for appending operation), make sure tail's next is NULL
    int size; 
    pthread_mutex_t* filetable_mutex; // mutex for the file table
}fileTable_t;




fileTable_t* filetable_init();

fileEntry_t* filetable_createFileEntry();

fileEntry_t* filetable_searchFileByName(fileTable_t* tablePtr, char* filename);

fileEntry_t* filetable_searchFileByNameWithoutMutex(fileEntry_t* head, char* filename);

int filetable_deleteFileEntryByName(fileTable_t* tablePtr, char* filename);

void filetable_appendFileEntry(fileTable_t* tablePtr, fileEntry_t* newEntryPtr);

void filetable_printFileTable(fileTable_t* tablePtr);

int filetable_updateFile(fileEntry_t* oldEntryPtr, fileEntry_t* newEntryPtr, pthread_mutex_t* tablemutex);

void filetable_destroy(fileTable_t *tablePtr);

fileTable_t* create_local_filetable(char* root_dir);

int filetable_AddIp2Iplist(fileEntry_t* entry, char* peerip, pthread_mutex_t* tablemutex);

int filetable_deleteIpfromIplist(fileEntry_t* entry, char* peerip, pthread_mutex_t* tablemutex);

int filetable_deleteIpfromAllEntries(fileTable_t* table, char* peerip);

char* filetable_convertFileEntriesToArray(fileEntry_t* entry, int num, pthread_mutex_t* tablemutex);

fileEntry_t* filetable_convertArrayToFileEntires(char* buf, int num);

fileTable_t* filetable_convertEntriesToFileTable(fileEntry_t* head_file);


#endif

