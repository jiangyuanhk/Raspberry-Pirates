
#include "peertable.h"
#include "filetable.h"


//client states used in FSM
#define	CLOSED 1
#define	SYNSENT 2
#define	CONNECTED 3
#define	FINWAIT 4





/* pkt from tracker to peer */
typedef struct segment_tracker {
// time interval that the peer should sending alive message periodically int interval;
	int heartbeatinterval;
// piece length
	int piece_len;

	int filetablesize;

	fileEntry_t* filetableHeadPtr;//array, by converting linkedlist of fileEntries

} ptp_tracker_t;



/* pkt: peer to tracker */
typedef struct segment_peer {
	int type;
	// the peer ip address sending this packet
	char peer_ip[IP_LEN];

	// listening port number in p2p
	int port;

	int filetablesize;

	fileEntry_t*  filetableHeadPtr;//array, by converting linkedlist of fileEntries
}ptp_peer_t;



/****** tracker side APIs ******/
int pkt_tracker_recvPkt(int connection, ptp_peer_t* pkt);
int pkt_tracker_sendPkt(int connection, ptp_tracker_t* pkt);



/****** peer side receive and send ******/
int pkt_peer_recvPkt(int connection, ptp_tracker_t* pkt);
int pkt_peer_sendPkt(int connection, ptp_peer_t* pkt);





ptp_tracker_t* pkt_create_trackerPkt();
ptp_peer_t* pkt_create_peerPkt();


void pkt_config_trackerPkt(ptp_tracker_t* pkt,  int heartbeatinterval, int piece_len, int filetablesize, fileEntry_t* filetableHeadPtr);
void pkt_config_peerPkt(ptp_peer_t* pkt,  int type, char* peer_ip, int port, int filetablesize, fileEntry_t* filetableHeadPtr);




