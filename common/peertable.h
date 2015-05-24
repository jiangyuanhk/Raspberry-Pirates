#include <pthread.h>
#include "constants.h"



typedef struct peerEntry {
    // Remote peer IP address, 16 bytes.
    char ip[IP_LEN];
    
    char file_name[FILE_NAME_MAX_LEN];  //peer: Current downloading file name
                                       // trakcer: dont care
    
    //Timestamp of current downloading file.
    unsigned long timestamp;       //peer:     TODO: not sure ? 
                                    //tracker:  latest alive timestamp of this peer
    //TCP connection to this remote peer.
    int sockfd;
    //Pointer to the next peer, linked list.
    struct peerEntry *next;
} peerEntry_t;



typedef struct peer_peertable{
    peerEntry_t* head;
    peerEntry_t* tail;
    int size;
    pthread_mutex_t* peertable_mutex;
}peerTable_t;








// This initializes the peer table.
peerTable_t* peerTable_init();

// This method creates a table entry and adds it to the end of the table.
// Note that sockfd is initially -1 and should be -1 whenever disconnected.
int peerTable_addEntry(peerTable_t *table, peerEntry_t* entry);
peerEntry_t* peertable_createEntry(char* ip, int sockfd);


// This method removes a table entry given the IP of the node to delete. Also fixes next pointers.
// Returns 1 on success, -1 on failure.
int peertable_deleteEntryByIp(peerTable_t *table, char* ip);

// This method deletes the whole table, freeing memory, etc.
// Returns 1 on success, -1 on failure.
void peertable_destroy(peerTable_t *table);











