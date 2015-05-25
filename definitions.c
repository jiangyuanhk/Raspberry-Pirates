

/***************** packets for communication between peer & server, peer & peer*******************/


/* pkt from tracker to peer */
typedef struct segment_tracker {
// time interval that the peer should sending alive message periodically int interval;
	int interval;
// piece length
	int piece_len;
// file table of the tracker -- your own design
	fileTable_t filetable;
} ptp_tracker_t;




/* pkt from peer to tracker */
typedef struct segment_peer {
	// protocol length
	int protocol_len;
	// protocol name
	char protocol_name[PROTOCOL_LEN + 1];
	// packet type : register, keep alive, update file table
	int type;
	// reserved space, you could use this space for your convenient, 8 bytes by default
	char reserved[RESERVED_LEN];
	// the peer ip address sending this packet
	char peer_ip[IP_LEN];
	// listening port number in p2p
	int port;

	fileTable_t filetable;
}ptp_peer_t;






/********************** Peer Table Structure ******************/



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




/********************** File Table Structure ******************/



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

