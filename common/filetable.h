

#include "constants.h"
#include <pthread.h>



/**
 * each file is represented as a fileEntry
 */
typedef struct fileEntry{
 //the size of the file
 int size;
 //the name of the file, must be unique in the same directory
 char *name;
 //the timestamp when the file is modified or created
 unsigned long int timestamp;
 //pointer to build the linked list
 struct fileEntry *pNext;

 char iplist[MAX_PEER_NUM][IP_LEN]; //tracker:  this is a list of peers' ips posessing the file
                                    //peer:     only contains ip of peer itself, put it in iplist[0]

 int peerNum;

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
   


/**
 * search fileEntry in the fileTable by name, return NULL if cannot find
 * @param  filename [headPtr of the fileTable to be searched]
 * @return          [description]
 */
fileEntry_t* filetable_searchFileByName(fileTable_t* tablePtr, char*  filename);


/**
 * delete fileEntry in the fleTable by name, return 1 if success, -1 if not
 * @param  headPtr  [description]
 * @param  filename [description]
 * @return          [description]
 */
int filetable_deleteFileEntryByName(fileTable* tablePtr, char* filename);


/**
 * append the newEntry to the end of the fileTable
 * @param headPtr     [description]
 * @param newEntryPtr [description]
 */
void filetable_appendFileEntry(fileTable_t* tablePtr, fileEntry_t* newEntryPtr);


/**
 * print the fileTale along the way for ease of debugging and checking
 */
void filetable_printFileTable(fileTable_t* tablePtr);



/**
 * update oldEntry in fileTable using newEntry
 * @param filetablePtr [description]
 * @param oldEntryPtr  [description]
 * @param newEntryPtr  [description]
 */
void filetable_filetable_updateFile(fileTable_t* filetablePtr, fileEntry_t* oldEntryPtr, fileEntry_t* newEntryPtr);



/**
 * get the table size
 * @param  tablePtr [description]
 * @return          [description]
 */
int filetable_getTableSize(fileTable_t* tablePtr);





void filetable_destroy(fileTable_t* tablePtr);



















