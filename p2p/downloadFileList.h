/** To keep track of ongoing downlaoding files in the peer side */


#include "../common/constants.h"

#include <pthread.h>

/* An entry in files downloading list */
typedef struct DLLEntry{
    char filename[FILE_NAME_MAX_LEN];
    struct DLLEntry* next;
} DLLEntry_t;

/* List of files that are in the process of being downloaded */
typedef struct DLL{
    DLLEntry* head;
    DLLEntry* tail;
    int size;
    pthread_mutex_t* mutex;
} DLL_t;





DLL_t* DLL_initList();

DLLEntry_t*	DLL_createEntry();


/**
 * add new file to the download list
 * if being downloaded before add, return -1, otherwise return 1 
 */
int DLL_addEntry(DLL_t* list, char* filename);

/**
 * tell if entry is already inside the list, entry indentified by name
 * return 1 if exists already, -1 if is new
 */
int DLL_existEntry(DLL_t* list, char* filename);



int DLL_removeEntry(DLL_t* list, char* filename);


int DLL_destroy(DLL_t* list);