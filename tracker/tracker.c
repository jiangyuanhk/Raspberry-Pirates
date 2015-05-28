

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
#include "../common/pkt.h"
#include "../common/filetable.h"
#include "../common/peertable.h"
#include "../common/utils.h"





fileTable_t* myFileTablePtr;
peerTable_t* myPeerTablePtr;

int svr_sd; // trakcer side socket binded with HANDSHAKE_PORT

/**
 * construct a broadcast packet and send fileTable updates to all peers
 */
void broadcastFileTable(){
 	
	//create a pkt to broadcast
 	ptp_tracker_t* broadcast = pkt_create_trackerPkt(); 
 	// config reponse
 	pkt_config_trackerPkt(broadcast, HEARTBEAT_INTERVAL, PIECE_LENGTH, myFileTablePtr->size, myFileTablePtr->head);


	//send the pkt_response to all peers (braodcasting)
	peerEntry_t* iter = myPeerTablePtr->head;
 	while(iter != NULL){
 		pkt_tracker_sendPkt(iter->sockfd, broadcast);
 		iter = iter->next;
 	}
 	//the actual linkedlist of Entries belongs to myFileTable, do not free
 	//only free broadcast
 	free(broadcast);
 }



/**
 * handshake thread: to consistently receive messages from peers, respond if needed, by using tracker-peer handshake protocal defined in pkt.c
 * @param  arg [the TCP connection identifier between a specific peer and the centralized trakcer ]
 * @return     [void]
 *
 * sudo code:
 *
 * keep receiving message on the TCP conection
 * 		case KEEPALIVE:
 *   		find the peer entry in tracker's peerTable (must be exactly only one entry)
 *   		update the peer's timestamp to current time
 *
 * 		case FILEUPDATE:
 * 			for each file entry in packet's fileTable:
 * 				search through tracker's fileTable
 * 				if it is a new file: 
 * 					add file to file table	
 * 				elif the file exists in tracker's fileTable:
 * 					if (peer has a newer version):
 * 						update tracker's fileEntry by peer's fileEntry (update timestamp, size)
 * 						
 *
 * 					elif (peer has an older version):
 * 						do nothing (becasue the new table would be eventaully broadcast later)
 * 							
 *
 *
 * 					else (same version)
 * 						add peer to the fileEntry's iplist if possible
 *
 * 			for each file entry in tracker's fileTable:
 * 				search through packet's fileTable:
 * 					if packet's fileTable does not have it: (search by name)
 * 						delete the entry from tracker's fileTable
 * 					elif packet's fileTable HAVE it: (search by name)
 * 						do nothing, becasue it must be synced in the upper loops
 * 
 */
void *handshake(void* arg){
 	
 	ptp_peer_t pkt_recv;
 	memset(&pkt_recv, 0, sizeof(ptp_peer_t));

	int connfd = *(int*) arg;

	while(1) {
		pkt_tracker_recvPkt(connfd, &pkt_recv);

		int type = pkt_recv.type;
		switch(type) {
			case KEEPALIVE:
			{
				peerEntry_t* tobeRefreshed = peertable_searchEntryByIp(myPeerTablePtr, pkt_recv.peer_ip);
				peertable_refreshTimestamp(tobeRefreshed);
				break;
			}
			case FILEUPDATE:
			{
				int needBroadCast = -1; 
				int i;
				//for each file entry in packet's fileTable:
				fileEntry_t* iter = pkt_recv.filetableHeadPtr;
				while(iter != NULL){
					//tracker's fileEntry found with same name(NULL if not found)
					fileEntry_t* res = filetable_searchFileByName(myFileTablePtr->head, iter->name);
					if(res == NULL){
						//if it is a new file: 
 						//add file to file table	
						filetable_appendFileEntry(myFileTablePtr, iter);
						needBroadCast = 1;

					} else {
						//the entry exists already
						if(iter->timestamp > res->timestamp){
							//if peer has a newer version
							//update tracker's fileEntry by peer's fileEntry 
							filetable_updateFile(res, iter, myFileTablePtr->filetable_mutex);
							
						} else if (iter->timestamp == res->timestamp){

							// if peer and tracker has the same version of the file 
							// add peerip to the fileEntry's iplist if possible
							if(filetable_AddIp2Iplist(res, iter->iplist[0], myFileTablePtr->filetable_mutex) > 0){
								needBroadCast = 1;
							}
							//only when case falls here, we do not need to broadcast, meaning every entry's every fileld are unchanged

						} else {
							// peer has an older version
							// wait for later broadcast
							needBroadCast = 1;

						}


					}

					iter = iter -> next;
				}

				//for each file entry in tracker's fileTable
				iter = myFileTablePtr->head;
				while(iter != NULL){
					//search through packet's fileTable (by name)
					fileEntry_t* res = filetable_searchFileByName(pkt_recv.filetableHeadPtr, iter->name);
					if(res != NULL){
						// if packet's fileTable does not have it
						// delete the entry from tracker's fileTable
						filetable_deleteFileEntryByName(myFileTablePtr, iter->name);
						needBroadCast = 1;
					}

					iter = iter -> next;
				}


				//at this time we finish sync fileTables between trakcer and server
				//begin to broadcast trakcer's new fileTable
				if(needBroadCast){
					broadcastFileTable();
				}

				break;

			}
			case REGISTER:
				printf("%s: error: aren't suppposed to receive REGISTER here\n", __func__);
				break;
		}
	}
}





/**
 * Periodically check if some peer is dead (DEAD_PEER_TIMEOUT)
 * remove dead peer from peerTable, remove peerip from fileTable
 */
void* monitorAlive(void* arg){
	while(1){
		//check periodically to prevent CPU burning...
		sleep(MINITOR_ALIVE_INTERVAL);
		
		//check the peerTable and remove all the dead peers 
		if (myPeerTablePtr->size > 0) {
			peerEntry_t* iter = myPeerTablePtr->head;

			//obtain current Time
			unsigned long currentTime = getCurrentTime();
			while(iter != NULL){
			//loop through peerTable
			//check each peer if dead or alive
				if(currentTime - iter->timestamp > DEAD_PEER_TIMEOUT){
					//if dead, delete this peer from table
					peerTable_deleteEntryByIp(iter->ip);
					//if dead, also remove this peerip from any entry's iplist in the table
					filetable_deleteIpfromAllEntries(myPeerTablePtr, iter->ip);
				}
			}
		}

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


// Cleanup data structures and any other memory allocations
// Register with SIGINT, so called when iterrupt the program
// see main loop 
void trackerStop() {
    // Free peer table and filetable
    peertable_destroy(myPeerTablePtr);
    filetable_destroy(myFileTablePtr);
    //close the socket binded with HANDSHAKE_PORT
    close(svr_sd);
}



/**
 * 1. initialize a peertable and a filetable
 * 2. create a socket binded with HANDSHAKE_PORT 
 * 3. create a MonitorAlive thread to periodically check the last alive timestamp of peers, remove those timeout peers
 * 4. register cleanup method when interrupt (SIGINT)
 * 5. keep listen to the socket binded with HANDSHAKE_PORT
 * 			if receive RESIGSTER pkt:
 * 				1. create a new peerEntry using REGISTER's ip, REGISTER's sockfd, and currentTime;
 * 				2. insert the new peerEntry into table (assert: no peer has same ip as this new peer before insertion)
 * 				3. send a response (with: HEARTBEAT_INTERVAL, FILEPIECE_LEN, filetable) back to peer for setup
 * 				4. create a handshake thread to handle messages from this particular peer
 */


 int main() {

	//1. initialize a peertable and a filetable
 	myPeerTablePtr = peertable_init();
 	myFileTablePtr = filetable_init();


 	//2. create a socket on HANDSHAKE_PORT
 	svr_sd = create_server_socket(HANDSHAKE_PORT);
 	assert(svr_sd >= 0);


	//3. start a thread to minitor & accept alive message from online peers periodically
	//remove dead peers if timeout occurs
 	pthread_t minitorAlive_thread;
 	pthread_create(&minitorAlive_thread, NULL, monitorAlive, NULL);


 	//4. register cleanup method when stop
 	signal(SIGINT, trackerStop);



	ptp_peer_t pkt_recv;// the packet received on HANDSHAKE_PORT

	//5. keeps listening on the socket binded with HANDSHAKE_PORT
	while(1) {

		//receive the REGISTER pkt if any
		struct sockaddr_in client_addr;
		socklen_t length = sizeof(client_addr);
		int connfd = accept(svr_sd, (struct sockaddr*) &client_addr, &length);
		assert(connfd >= 0);


		pkt_tracker_recvPkt(connfd, &pkt_recv);


		if(pkt_recv.type == REGISTER) {

			//create a new peerEntry using 1. REGISTER's ip, 2. connfd: denoting the TCP connection between this peer and tracker;
			peerEntry_t* peerEntry = peertable_createEntry(pkt_recv.peer_ip, connfd);

			// insert the new peerEntry into table (assert: no peer has same ip as this new peer before insertion)
			assert(peertable_searchEntryByIp(myPeerTablePtr, peerEntry -> ip) == NULL);
			peertable_addEntry(myPeerTablePtr, peerEntry);

			//create a pkt to send back to peer, for peer to set up itself
			//the pkt contains info: 1. HEATBEAT_INTERVAL 2. PIECE_LENGTH 3. trakcer's fileTable(including size and the linkedlist)
			ptp_tracker_t* setup = pkt_create_trackerPkt();
			pkt_config_trackerPkt(setup, HEARTBEAT_INTERVAL, PIECE_LENGTH, myFileTablePtr->size, myFileTablePtr->head);

			//send the configured pkt --> peer
			pkt_tracker_sendPkt(connfd, setup);
			

 			//create a handshake thread to handle messages from this particular peer
			//receive messages from this peer and respond if needed
			//by using the peer-tracker handshake protocal
			pthread_t handshake_thread;
			pthread_create(&handshake_thread, NULL, handshake, &connfd);
			
		}


	}
}










