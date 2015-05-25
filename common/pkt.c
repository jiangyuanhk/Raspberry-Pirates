
#include "peertable.h"
#include "filetable.h"
#include "pkt.h"
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <string.h>


int pkt_tracker_recvPkt(int connfd, ptp_peer_t* pkt){

	int type, port, filetablesize;
	char* peer_ip = (char*)malloc(IP_LEN * sizeof(char));
	fileEntry_t* head = NULL;

	if(recv(connfd, &type, sizeof(int), 0) < 0){
		printf("err in %s: failed to receive type\n", __func__ );
		return -1;
	}

	if(recv(connfd, peer_ip, IP_LEN * sizeof(char), 0) < 0){
		printf("err in %s: failed to receive peer_ip\n", __func__ );
		return -1;
	}

	if(recv(connfd, &port, sizeof(int), 0) < 0){
		printf("err in %s: failed to receive type\n", __func__ );
		return -1;
	}

	if(recv(connfd, &filetablesize, sizeof(int), 0) < 0){
		printf("err in %s: failed to receive type\n", __func__ );
		return -1;
	}


	if(filetablesize > 0){
		//total number of bytes needed for buffer
		int totalBytes = filetablesize * sizeof(fileEntry_t);

		char* buf = (char*) malloc(totalBytes);
		if(recv(connfd, buf, totalBytes, 0) < 0) {
			printf("err in %s: failed to receive arraylist of entries\n", __func__);
			return -1;
		}
		head = filetable_convertArrayToFileEntires(buf, filetablesize);
	}




	//assemble the pieces
	pkt->type = type;
	memcpy(pkt->peer_ip, peer_ip, IP_LEN * sizeof(char));
	pkt->port = port;
	pkt->filetablesize = filetablesize;
	pkt->filetableHeadPtr = head;
	return 1;

}






int pkt_peer_sendPkt(int connfd, ptp_peer_t* pkt){
	


	if(send(connfd, &(pkt->type), sizeof(int), 0) < 0){
		printf("err in %s: send type failed\n", __func__);
		return -1;
	}


	if(send(connfd, &(pkt->peer_ip), sizeof(pkt->peer_ip), 0) < 0){
		printf("err in %s: send peerip failed\n", __func__);
		return -1;
	}


	if(send(connfd, &(pkt->port), sizeof(int), 0) < 0){
		printf("err in %s: send entry number failed\n", __func__);
		return -1;
	}


	if(send(connfd, &(pkt->filetablesize), sizeof(int), 0) < 0){
		printf("err in %s: send filetablesize failed\n", __func__);
		return -1;
	}


	if(pkt->filetablesize > 0){
		int totalBytes = (pkt->filetablesize) * sizeof(fileEntry_t);
		char* buf = filetable_convertFileEntriesToArray(pkt->filetableHeadPtr, pkt->filetablesize);
		if(send(connfd, buf, totalBytes, 0) < 0){
			printf("err in %s: send arraylist of entries failed\n", __func__);
			return -1;
		}
	}

	return 1;

}




int pkt_tracker_sendPkt(int connfd, ptp_tracker_t* pkt){

	if(send(connfd, &(pkt->heartbeatinterval), sizeof(int), 0) < 0){
		printf("err in %s: send heartbeatinterval failed\n", __func__);
		return -1;
	}


	if(send(connfd, &(pkt->piece_len), sizeof(int), 0) < 0){
		printf("err in %s: send piece_len failed\n", __func__);
		return -1;
	}


	if(send(connfd, &(pkt->filetablesize), sizeof(int), 0) < 0){
		printf("err in %s: send filetablesize failed\n", __func__);
		return -1;
	}



	if(pkt->filetablesize > 0){
		int totalBytes = (pkt->filetablesize) * sizeof(fileEntry_t);
		char* buf = filetable_convertFileEntriesToArray(pkt->filetableHeadPtr, pkt->filetablesize);
		if(send(connfd, buf, totalBytes, 0) < 0){
			printf("err in %s: send arraylist of entries failed\n", __func__);
			return -1;
		}
	}

	return 1;

}



int pkt_peer_recvPkt(int connfd, ptp_tracker_t* pkt){


	int heartbeatinterval, piece_len, filetablesize;
	fileEntry_t* head = NULL;

	if(recv(connfd, &heartbeatinterval, sizeof(int), 0) < 0){
		printf("err in %s: failed to receive heartbeatinterval\n", __func__ );
		return -1;
	}

	if(recv(connfd, &piece_len, sizeof(int), 0) < 0){
		printf("err in %s: failed to receive piece_len\n", __func__ );
		return -1;
	}

	if(recv(connfd, &filetablesize, sizeof(int), 0) < 0){
		printf("err in %s: failed to receive filetablesize\n", __func__ );
		return -1;
	}


	if(filetablesize > 0){
		//total number of bytes needed for buffer
		int totalBytes = filetablesize * sizeof(fileEntry_t);

		char* buf = (char*) malloc(totalBytes);
		if(recv(connfd, buf, totalBytes, 0) < 0) {
			printf("err in %s: failed to receive arraylist of entries\n", __func__);
			return -1;
		}
		head = filetable_convertArrayToFileEntires(buf, filetablesize);
	}

	//assemble the pieces
	pkt->heartbeatinterval = heartbeatinterval;
	pkt->piece_len = piece_len;
	pkt->filetablesize = filetablesize;
	pkt->filetableHeadPtr = head;


}
