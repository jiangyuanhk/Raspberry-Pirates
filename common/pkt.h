
#ifndef PKT_H
#define PKT_H

#include "peertable.h"
#include "filetable.h"

#define REGISTER 0
#define KEEP_ALIVE 1
#define FILE_UPDATE 2

/* pkt from tracker to peer */
typedef struct segment_tracker {
	int heartbeatinterval; 			     // time interval that the peer should sending alive message periodically int interval;
	int piece_len;					         // piece length
	int filetablesize;				       // number of entries in the filetable
	fileEntry_t* filetableHeadPtr;	 //array, by converting linkedlist of fileEntries
} ptp_tracker_t;

/* pkt: peer to tracker */
typedef struct segment_peer {
	int type;						              // type of segment being sent
	char peer_ip[IP_LEN]; 			      // the peer ip address sending this packet
	int port;						              // listening port number in p2p
	int filetablesize;		            // number of entries in the file table
	fileEntry_t* filetableHeadPtr;    //array, by converting linkedlist of fileEntries
}ptp_peer_t;


/****** tracker side APIs ******/
int pkt_tracker_recvPkt(int connection, ptp_peer_t* pkt);
int pkt_tracker_sendPkt(int connection, ptp_tracker_t* pkt, pthread_mutex_t* mutex);

/****** peer side receive and send ******/
int pkt_peer_recvPkt(int connection, ptp_tracker_t* pkt);
int pkt_peer_sendPkt(int connection, ptp_peer_t* pkt, pthread_mutex_t* mutex);

ptp_tracker_t* pkt_create_trackerPkt();
ptp_peer_t* pkt_create_peerPkt();

void pkt_config_trackerPkt(ptp_tracker_t* pkt,  int heartbeatinterval, int piece_len, int filetablesize, fileEntry_t* filetableHeadPtr);
void pkt_config_peerPkt(ptp_peer_t* pkt,  int type, char* peer_ip, int port, int filetablesize, fileEntry_t* filetableHeadPtr);

#endif

