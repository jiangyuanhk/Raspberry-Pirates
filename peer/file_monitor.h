// #ifndef FILE_MONITOR_H
// #define FILE_MONITOR_H

#include <pthread.h>

#include "../common/constants.h"
#include "../common/filetable.h"


typedef struct fileBlockEntry {
	char file_name[FILE_NAME_MAX_LEN];
	struct fileBlockEntry* next;
} file_t;


typedef struct {
	int size;
	struct fileBlockEntry* head;
	struct fileBlockEntry* tail;
	pthread_mutex_t* mutex;
} blockList_t;


char* read_config_file(char* filename);

fileTable_t* create_local_filetable(char* root_dir);

blockList_t* blocklist_init();

void add_file_to_blocklist(blockList_t* blocklist, char* filepath);

int find_in_blocklist(blockList_t* blocklist, char* filepath);

int remove_from_blocklist(blockList_t* blocklist, char* filepath);


// #endif
