
#include "peertable.h"
#include "filetable.h"


//client states used in FSM
#define	CLOSED 1
#define	SYNSENT 2
#define	CONNECTED 3
#define	FINWAIT 4





/* The packet data structure sending from tracker to peer */
typedef struct segment_tracker {
// time interval that the peer should sending alive message periodically int interval;
	int interval;
// piece length
	int piece_len;
// file number in the file table -- optional
	int file_table_size;
// file table of the tracker -- your own design
	Node* file_table[MAX_FILE_NUM];
} ptp_tracker_t;



/* The packet data structure sending from peer to tracker */
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
	// the number of files in the local file table -- optional
	int file_table_size;
	// file table of the client -- your own design
	Node file_table[MAX_FILE_NUM];
}ptp_peer_t;



/****** tracker side receive and send ******/
int tracker_recvPkt(int connection, ptp_peer_t * pkt);
int tracker_sendPkt(int connection, ptp_tracker_t * pkt);


/****** peer side receive and send ******/
int client_recvPkt(int connection, ptp_tracker_t* pkt);
int client_sendPkt(int connection, ptp_peer_t* pkt);





