#include <pthread.h>



//Peer-side peer table, represented as a linkedlist of entries
//each peer_peer_t represents a "peer"
typedef struct peer_peertable_entry {
    // Remote peer IP address, 16 bytes.
    char ip[IP_LEN];
    //Current downloading file name.
    char file_name[FILE_NAME_MAX_LEN];
    
    //Timestamp of current downloading file.
    unsigned long file_time_stamp;
    //TCP connection to this remote peer.
    int sockfd;
    //Pointer to the next peer, linked list.
    struct _peer_side_peer_t *next;
} peer_peertable_entry_t;



typedef struct peer_peertable{
    peer_peertable_entry_t* headPtr;
    peer_peertable_entry_t* tailPtr;
    int size;
    pthread_mutex_t* mutexPtr;
}peer_peertable_t;








//tracker side peer table, represented as a linkedlist of entries
//each trakcer_peer_t represents a "peer"
typedef struct tracker_peertable_entry {
    //Remote peer IP address, 16 bytes.
    char ip[IP_LEN];
    //Last alive timestamp of this peer.
    unsigned long last_time_stamp;
    //TCP connection to this remote peer.
    int sockfd;
    //Pointer to the next peer, linked list.
    struct tracker_peertable_entry *next;
} tracker_peertable_entry_t;




typedef struct tracker_peertable {
    tracker_peertable_entry* head;
    tracker_peertable_entry* tail;
    int size;
    pthread_mutex_t* mutexPtr; 
} trakcer_peer_table_t;








/**trakcer side APIs **/

void tracker_initPeerTable();
void peer_initPeerTable();

void updatePeerTable(tracker_peer_t* entry);
void appendPeerTable(tracker_peer_t* entryPtr);



/** peer side APIs */





