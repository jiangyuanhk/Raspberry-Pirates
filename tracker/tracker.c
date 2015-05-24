



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <assert.h>
// #include "tracker.h"
#include "../common/pkt.h"
#include "../common/filetable.h"
#include "../common/peertable.h"
#include "../common/utils.h"





fileTable_t* myFileTablePtr;




/**
 * update file table on the tracker, based on the received FILEUPDATE packet from the peer
 * @param pktPtr [FILEUPDATE packet received from peer]
 */
void updateFileTable(ptp_peer_t* pktPtr){
	
	int peer_tableSize = pktPtr->file_table_size;
	int i;


	Node* iter = filetable_headPtr;

	while(iter != NULL){
		//for every file in tracker, check if it is in peer's filetable
		//if it is, then update accordingly
		//if it is not, delete it from trakcer's filetable
		

		if(findFileEntryByName(filetable_headPtr, iter->name) == NULL){
			deleteFileEntryByName(filetable_headPtr, iter->)
		}




	}




}










void broadcastFileTable(){
	tracker_peer_t* peerIter = tracker_peertable_headPtr;
	

	//create a pkt as a reponse to be broadcast to all peers
	ptp_tracker_t pkt_response;
	memset(&pkt_response, 0, sizeof(ptp_peer_t));
	pkt_response.interval = HEARTBEAT_INTERVAL;
	pkt_response.piece_len = PIECE_LENGTH;
	pkt_response.file_table_size = getFileTableSize();


	//copy tracker side filetable to pkt_response
	pthread_mutex_lock(filetable_mutext);
	Node* temp = filetable_headPtr;
	int i = 0;
	while(temp != NULL) {
		memcpy(pkt_response.filetable[i], temp, sizeof(Node));
		i++;
		temp = (Node*)temp->pNext;
	}
	pthread_mutex_unlock(filetable_mutext);


	//send the pkt_response to all peers (braodcasting)
	while(peerItr != NULL){
		if(tracker_sendPkt(peerIter->sockfd, pkt_response) < 0){
			//TODO: if failed then ...delete the entry
		} else {
			peerItr = peerItr->next;
		}
	}

	free(pkt_response);
}










	void *handshake_handler(void* arg){
		ptp_peer_t pkt_recv;
	tracker_peer_t* entryPtr = (tracker_peer_t*) arg; // one peer in the peertable


	while(1){
		memset(&pkt_recv, 0, sizeof(ptp_peer_t));

		tracker_recvPkt(entryPtr->sockfd, &pkt_recv);

		if(pkt_recv.peer_ip != "" && strcmp(pkt_recv.peer_ip, entryPtr->ip) == 0 ){

			if(pkt_recv.type == FILEUPDATE){
				//update file table
				updateFileTable(&pkt_recv);
				//broadcast file table to all peers
				broadcastFileTable();
			}
			if(pkt_recv.type == KEEPALIVE){
				//search for the peer with ip address = pkt_recv.peer_ip
				//update this peer's timestamp
				unsigned long curTime = getCurrentTime();
				updatePeerTimeStamp(pkt_recv.peer_ip, curTime);
			}
		}	
	}
}






void* monitorAlive(void* arg){
	while(1){
		sleep(MINITOR_ALIVE_INTERVAL);
		unsigned long curTime = getCurrentTime();
		//compare current time with each file's last_time_stamp
		//if the difference larger than timeout value, delete the corresponding entry
		deleteDeadPeers(unsigned long curTime);//TODO
	}
}






//create a particular server socket using given port number
int create_server_socket(int portNum) {
	printf("%s called \n", __func__);

	int tcpserv_sd;
	struct sockaddr_in tcpserv_addr;

	//create a server socket, and configure its port
	tcpserv_sd = socket(AF_INET, SOCK_STREAM, 0); 
	if(tcpserv_sd<0) 
		return -1;
	memset(&tcpserv_addr, 0, sizeof(tcpserv_addr));
	tcpserv_addr.sin_family = AF_INET;
	tcpserv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	tcpserv_addr.sin_port = htons(portNum);


	if(bind(tcpserv_sd, (struct sockaddr *)&tcpserv_addr, sizeof(tcpserv_addr))< 0)
		return -1; 
	if(listen(tcpserv_sd, 1) < 0) 
		return -1;

	printf("%s finished successfully\n", __func__);
	return tcpserv_sd;
}



/**
 * create an entry in the tracker's peertable according to the REGISTER packet receieved from peer
 * @param pkt   [REGISTER packet from peer]
 * @param entry [created entry in the tracker-side peer table]
 * @param connfd [the conneciton between this trakcer and the peer from where the pkt is from]
 */
 peertable create_peer_table_entry(ptp_peer_t pkt, int connfd, tracker_peer_t* entryPtr){
 	memset(entryPtr, 0, sizeof(tracker_peer_t));
 	memcpy(entryPtr->ip, pkt.peer_ip, IP_LEN);
 	entryPtr->time_stamp = getCurrentTime();

 	entryPtr->sockfd = connfd;
 	entryPtr->next = NULL;

 }


/**
 * 1. initialize a peertable and a filetable
 * 2. create a HANDSHAKE_PROT and listens to it
 * 2. 
 */


 int main() {

	//create a socket on HANDSHAKE_PORT
 	svr_sd = create_server_socket(HANDSHAKE_PORT);
 	assert(svr_sd >= 0);

 	initPeerTable();
 	myFileTablePtr = filetable_init();

	//start a thread to minitor & accept alive message from online peers periodically
	//remove dead peers if timeout occurs
 	pthread_t minitorAlive_thread;
 	pthread_create(&minitorAlive_thread, NULL, monitorAlive, NULL);



	ptp_peer_t pkt_recv;// the packet received on HANDSHAKE_PORT

	//keeps listening on the socket binded with HANDSHAKE_PORT
	while(1) {

		//receive the REGISTER pkt if any
		struct sockaddr_in client_addr;
		socklen_t length = sizeof(client_addr);
		int connfd = accept(svr_sd, (struct sockaddr*) &client_addr, &length);
		assert(connfd >= 0);


		tracker_recvPkt(connfd, &pkt_recv);


		if(pkt_recv.type == REGISTER) {

			peer entry;
			create_peer_table_entry(pkt_recv, connfd, &entry);
			update_peer_table(&entry);

			//create a pkt to send back to peer, for peer to set up itself
			ptp_tracker_t pkt_response;
			memset(&pkt_response, 0, sizeof(ptp_peer_t));
			pkt_response.interval = HEARTBEAT_INTERVAL;
			pkt_response.piece_len = PIECE_LENGTH;
			pkt_response.file_table_size = getFileTableSize();


			//copy tracker side filetable to pkt_response
			pthread_mutex_lock(filetable_mutext);
			Node* temp = filetable_headPtr;
			int i = 0;
			while(temp != NULL) {
				memcpy(pkt_response.filetable[i], temp, sizeof(Node));
				i++;
				temp = (Node*)temp->pNext;
			}
			pthread_mutex_unlock(filetable_mutext);


			//send the configured pkt back to peer
			tracker_sendPkt(connfd, pkt_response);
			
			//receive messages from this peer and respond if needed
			//by using the peer-tracker handshake protocal
			pthread_t handshake_thread;
			pthread_create(&handshake_thread, NULL, handshake, &entry);
			
		}


	}
}