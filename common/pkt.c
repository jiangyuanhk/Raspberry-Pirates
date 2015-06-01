


#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "utils.h"
#include "peertable.h"
#include "filetable.h"
#include "pkt.h"

/************** SEND and RECV **********************************/

int pkt_tracker_recvPkt(int connfd, ptp_peer_t* pkt){

	int type, port, filetablesize;
	char* peer_ip = (char*)malloc(IP_LEN * sizeof(char));
	fileEntry_t* head = NULL;

  int num;

	if( (num = recv(connfd, &type, sizeof(int), 0)) <= 0) {
		printf("err in %s: failed to receive type\n", __func__ );
		return -1;
	}

	if( (num = recv(connfd, peer_ip, IP_LEN * sizeof(char), 0)) <= 0){
		printf("err in %s: failed to receive peer_ip\n", __func__ );
		return -1;
	}

	if( (num = recv(connfd, &port, sizeof(int), 0)) <= 0){
		printf("err in %s: failed to receive type\n", __func__ );
		return -1;
	}

	if( (num = recv(connfd, &filetablesize, sizeof(int), 0)) <= 0){
		printf("err in %s: failed to receive type\n", __func__ );
		return -1;
	}

  if(filetablesize > 0) {
    //total number of bytes needed for buffer
    int totalBytes = filetablesize * sizeof(fileEntry_t);
    int total_received = 0;
    int num_received = 0;

    //initialize buffer to read data into and another to store all data for list
    char* recv_buf = (char*) malloc(totalBytes);
    memset(recv_buf, 0, totalBytes);

    char* filetable_buf = (char*) malloc(totalBytes);
    memset(filetable_buf, 0, totalBytes);

    //lopp through and make sure receive the entire array
    while (total_received != totalBytes) {
      if( (num_received = recv(connfd, recv_buf, totalBytes, 0)) <= 0) {
        printf("err in %s: failed to receive arraylist of entries\n", __func__);
        return -1;
      }
      memcpy(&filetable_buf[total_received], recv_buf, num_received);
      total_received = total_received + num_received;
      memset(recv_buf, 0, num_received);
    }

    head = filetable_convertArrayToFileEntires(filetable_buf, filetablesize);
  }

	//assemble the pieces
	pkt->type = type;
	memcpy(pkt->peer_ip, peer_ip, IP_LEN * sizeof(char));
	pkt->port = port;
	pkt->filetablesize = filetablesize;
	pkt->filetableHeadPtr = head;
	return 1;
}


int pkt_peer_sendPkt(int connfd, ptp_peer_t* pkt, pthread_mutex_t* mutex) {
	


	if(send(connfd, &(pkt->type), sizeof(int), 0) <= 0){
		printf("err in %s: send type failed\n", __func__);
		return -1;
	}


	if(send(connfd, &(pkt->peer_ip), IP_LEN * sizeof(char), 0) <= 0){
		printf("err in %s: send peerip failed\n", __func__);
		return -1;
	}


	if(send(connfd, &(pkt->port), sizeof(int), 0) <= 0){
		printf("err in %s: send entry number failed\n", __func__);
		return -1;
	}


	if(send(connfd, &(pkt->filetablesize), sizeof(int), 0) <= 0){
		printf("err in %s: send filetablesize failed\n", __func__);
		return -1;
	}


	if(pkt->filetablesize > 0){
		int totalBytes = (pkt->filetablesize) * sizeof(fileEntry_t);
		char* buf = filetable_convertFileEntriesToArray(pkt->filetableHeadPtr, pkt->filetablesize, mutex);
		if(send(connfd, buf, totalBytes, 0) <= 0){
			printf("err in %s: send arraylist of entries failed\n", __func__);
			return -1;
		}
	}

	return 1;
}

int pkt_tracker_sendPkt(int connfd, ptp_tracker_t* pkt, pthread_mutex_t* mutex){

	if(send(connfd, &(pkt->heartbeatinterval), sizeof(int), 0) <= 0){
		printf("err in %s: send heartbeatinterval failed\n", __func__);
		return -1;
	}


	if(send(connfd, &(pkt->piece_len), sizeof(int), 0) <= 0){
		printf("err in %s: send piece_len failed\n", __func__);
		return -1;
	}


	if(send(connfd, &(pkt->filetablesize), sizeof(int), 0) <= 0){
		printf("err in %s: send filetablesize failed\n", __func__);
		return -1;
	}

	printf("Try to send as array \n");
	if(pkt->filetablesize > 0){
		int totalBytes = (pkt->filetablesize) * sizeof(fileEntry_t);
		char* buf = filetable_convertFileEntriesToArray(pkt->filetableHeadPtr, pkt->filetablesize, mutex);
		if(send(connfd, buf, totalBytes, 0) <= 0){
			printf("err in %s: send arraylist of entries failed\n", __func__);
			return -1;
		}
	}

	return 1;
}

int pkt_peer_recvPkt(int connfd, ptp_tracker_t* pkt){


	int heartbeatinterval, piece_len, filetablesize;
	fileEntry_t* head = NULL;

	if(recv(connfd, &heartbeatinterval, sizeof(int), 0) <= 0){
		printf("err in %s: failed to receive heartbeatinterval\n", __func__ );
		return -1;
	}

	if(recv(connfd, &piece_len, sizeof(int), 0) <= 0){
		printf("err in %s: failed to receive piece_len\n", __func__ );
		return -1;
	}

	if(recv(connfd, &filetablesize, sizeof(int), 0) <= 0){
		printf("err in %s: failed to receive filetablesize\n", __func__ );
		return -1;
	}

	if(filetablesize > 0) {

    //total number of bytes needed for buffer
    int totalBytes = filetablesize * sizeof(fileEntry_t);
    int total_received = 0;
    int num_received = 0;

    //initialize buffer to read data into and another to store all data for list
    char* recv_buf = (char*) malloc(totalBytes);
    memset(recv_buf, 0, totalBytes);
    
    char* filetable_buf = (char*) malloc(totalBytes);
    memset(filetable_buf, 0, totalBytes);

    //lopp through and make sure receive the entire array
    while (total_received != totalBytes) {
      if( (num_received = recv(connfd, recv_buf, totalBytes, 0)) <= 0) {
        printf("err in %s: failed to receive arraylist of entries\n", __func__);
        return -1;
      }
      memcpy(&filetable_buf[total_received], recv_buf, num_received);
      total_received = total_received + num_received;
      memset(recv_buf, 0, num_received);
    }

		head = filetable_convertArrayToFileEntires(filetable_buf, filetablesize);
	}

	//assemble the pieces
	pkt->heartbeatinterval = heartbeatinterval;
	pkt->piece_len = piece_len;
	pkt->filetablesize = filetablesize;
	pkt->filetableHeadPtr = head;
  return 1;
}


/********** CREATE **********************************/

ptp_tracker_t* pkt_create_trackerPkt(){
	ptp_tracker_t* pkt = (ptp_tracker_t*)malloc(sizeof(ptp_tracker_t));
	memset(pkt, 0, sizeof(ptp_tracker_t));
	return pkt;

}
ptp_peer_t* pkt_create_peerPkt(){
	ptp_peer_t* pkt = (ptp_peer_t*)malloc(sizeof(ptp_peer_t));
	memset(pkt, 0, sizeof(ptp_peer_t));
	return pkt;
}


// // Function that sends a register packet upon first login.
// int send_register_packet(int tracker_conn) {
//   int type = REGISTER;
//   int filetablesize = 0;
//   int port = P2P_PORT;

//   char* ip = malloc(IP_LEN);
//   memset(ip, 0, IP_LEN);
//   get_my_ip(ip);

//   if(send(tracker_conn, &(type), sizeof(int), 0) < 0){
//     printf("err in %s: send type failed\n", __func__);
//     return -1;
//   }

//   if(send(tracker_conn, ip, IP_LEN * sizeof(char), 0) < 0){
//     printf("err in %s: send peerip failed\n", __func__);
//     free(ip);
//     return -1;
//   }
//   free(ip);


//   if(send(tracker_conn, &(port), sizeof(int), 0) < 0){
//     printf("err in %s: send entry number failed\n", __func__);
//     return -1;
//   }

//   if(send(tracker_conn, &(filetablesize), sizeof(int), 0) < 0){
//     printf("err in %s: send filetablesize failed\n", __func__);
//     return -1;
//   }

//   return 1;
// }

/********* CONFIGURE ***********************************/

void pkt_config_trackerPkt(ptp_tracker_t* pkt,  int heartbeatinterval, int piece_len, int filetablesize, fileEntry_t* filetableHeadPtr){
	pkt->heartbeatinterval = heartbeatinterval;
	pkt->piece_len = piece_len;
	pkt->filetablesize = filetablesize;
	pkt->filetableHeadPtr = filetableHeadPtr;
}

void pkt_config_peerPkt(ptp_peer_t* pkt,  int type, char* peer_ip, int port, int filetablesize, fileEntry_t* filetableHeadPtr){
	pkt->type = type;
	strcpy(pkt->peer_ip, peer_ip);
	pkt->port = port;
	pkt->filetablesize = filetablesize;
	pkt->filetableHeadPtr = filetableHeadPtr;
}
