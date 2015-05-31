

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

//Global Variables
fileTable_t* myFileTablePtr;
peerTable_t* myPeerTablePtr;

// ptp_peer_t* pkt_recv; //pointer to the packet that will receive information from peers

int svr_sd; // tracker side socket binded with HANDSHAKE_PORT


/**
 * construct a broadcast packet and send fileTable updates to all peers
 */
void broadcastFileTable() {
 	printf("Broadcasting File Table to %d Peers\n", myPeerTablePtr -> size);
  printf("This is the file table we will be sending\n");
  filetable_printFileTable(myFileTablePtr);
	//create a pkt to broadcast
 	ptp_tracker_t* broadcast = pkt_create_trackerPkt(); 
 	// config reponse
 	pkt_config_trackerPkt(broadcast, HEARTBEAT_INTERVAL, PIECE_LENGTH, myFileTablePtr->size, myFileTablePtr->head);

	//send the pkt_response to all peers (broadcasting)
	peerEntry_t* iter = myPeerTablePtr->head;
 	while(iter != NULL){
    printf("Sending file table to ip: %s\n with sockfd: %d", iter -> ip, iter -> sockfd);
 		pkt_tracker_sendPkt(iter->sockfd, broadcast, myFileTablePtr -> filetable_mutex);
 		iter = iter->next;
 	}
  printf("Finished broadcasting table.\n");
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
		if (pkt_tracker_recvPkt(connfd, &pkt_recv) < 0) return 0;
    
    printf("Type: %d\n", pkt_recv.type);
    printf("File Table Size: %d\n", pkt_recv.filetablesize);

		int type = pkt_recv.type;
		switch(type) {
			case KEEP_ALIVE: {
        printf("Received a keep alive packet!\n");
				peerEntry_t* tobeRefreshed = peertable_searchEntryByIp(myPeerTablePtr, pkt_recv.peer_ip);
				peertable_refreshTimestamp(tobeRefreshed);
				break;
			}

			case FILE_UPDATE: {
        printf("Received a file update packet!\n");

        fileTable_t* peer_filetable = filetable_convertEntriesToFileTable(pkt_recv.filetableHeadPtr);
        filetable_printFileTable(peer_filetable);

				int needBroadCast = -1; 

        //Check if we need to add or update entries in the tracker file table

				//for each file entry in packet's fileTable:
				fileEntry_t* iter = peer_filetable -> head;

				while(iter != NULL) {
					
          fileEntry_t* entry = filetable_searchFileByName(myFileTablePtr, iter->file_name);

          // if the entry is null, it is a new file and needs to be added to the filetable and the ip the list of hosts with the file
          if(entry == NULL) {
            fileEntry_t* entry_to_add = malloc(sizeof(fileEntry_t));
            memcpy(entry_to_add, iter, sizeof(fileEntry_t));
            entry_to_add -> next = NULL;
						filetable_appendFileEntry(myFileTablePtr, entry_to_add);
            filetable_AddIp2Iplist(entry_to_add, pkt_recv.peer_ip, myFileTablePtr->filetable_mutex);
						needBroadCast = 1;
					} 

          // if the filetable entry has an outdated timestamp, the file has been updated
          // so need broadcast and update the file
          else if( (iter->timestamp) > (entry -> timestamp) ) {
            filetable_updateFile(entry, iter, myFileTablePtr->filetable_mutex);
            needBroadCast = 1;
          }

          // if the timestamps are the same, the file is updated at the peer so add to ip list for file
          // if the ip is already in the list for the file, do not broadcast again
          else if( (iter -> timestamp) == (entry -> timestamp) ) {
            
            //only broadcast if successfully add to the list, do not add/broadcast if already in list
            if (filetable_AddIp2Iplist(entry, pkt_recv.peer_ip, myFileTablePtr->filetable_mutex) > 0) {
              needBroadCast = 1;
            }
          }

          //otherwise the peer file is outdated compared to the tracker file table and needs to be updated
          else {
            needBroadCast = 1;
          }

					iter = iter -> next;
				}

        //Check if we need to delete an entries in the tracker file table
        //for each file entry in tracker's fileTable
        iter = myFileTablePtr->head;
    		
        while(iter != NULL){

    			fileEntry_t* entry = filetable_searchFileByName(peer_filetable, iter -> file_name);
    			
          //if the entry exists in the tracker filetable and not in the local table, remove the entry from the tracker table
          if(entry == NULL){
    				filetable_deleteFileEntryByName(myFileTablePtr, iter -> file_name);
    				needBroadCast = 1;
    			}

    			iter = iter -> next;
    		}

    		//at this time we finish sync fileTables between trakcer and server
    		//begin to broadcast tracker's new fileTable
    		if(needBroadCast){
    			broadcastFileTable();
    		}

        if (peer_filetable) {
          filetable_destroy(peer_filetable);
        }

    		break;

			}
			case REGISTER:
        sleep(5);
        printf("Received a register packet!\n");
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
		sleep(MONITOR_ALIVE_INTERVAL);
		
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
					peertable_deleteEntryByIp(myPeerTablePtr, iter->ip);
					//if dead, also remove this peerip from any entry's iplist in the table
					filetable_deleteIpfromAllEntries(myFileTablePtr, iter->ip);
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
	if(tcpserv_sd < 0) 
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
    //close the socket binded with HANDSHAKE_PORT
    close(svr_sd);
    // Free peer table and filetable
    peertable_destroy(myPeerTablePtr);
    filetable_destroy(myFileTablePtr);
    exit(0);
}


//Function to receive a file table from a peer for the first time to avoid deleting files in the tracker that should 
// be synced back to the local peer.
int initial_sync_with_peer(int conn) {
  ptp_peer_t pkt_recv;
  memset(&pkt_recv, 0, sizeof(ptp_peer_t));

  //if fail to receive the packet, return -1.
  if (pkt_tracker_recvPkt(conn, &pkt_recv) < 0) return -1;
    
  printf("Type: %d\n", pkt_recv.type);
  printf("File Table Size: %d\n", pkt_recv.filetablesize);

  //if receive the wrong kind of table, reset
  if (pkt_recv.type != FILE_UPDATE) return -1;
  printf("Received a file update packet!\n");

  fileTable_t* peer_filetable = filetable_convertEntriesToFileTable(pkt_recv.filetableHeadPtr);
  filetable_printFileTable(peer_filetable);

  //Check if we need to add or update entries in the tracker file table

  //for each file entry in packet's fileTable:
  fileEntry_t* iter = peer_filetable -> head;

  while(iter != NULL) {
    
    fileEntry_t* entry = filetable_searchFileByName(myFileTablePtr, iter->file_name);

    // if the entry is null, it is a new file and needs to be added to the filetable and the ip the list of hosts with the file
    if(entry == NULL) {
      fileEntry_t* entry_to_add = malloc(sizeof(fileEntry_t));
      memcpy(entry_to_add, iter, sizeof(fileEntry_t));
      entry_to_add -> next = NULL;
      filetable_appendFileEntry(myFileTablePtr, entry_to_add);
      filetable_AddIp2Iplist(entry_to_add, pkt_recv.peer_ip, myFileTablePtr->filetable_mutex);
    } 

    // if the filetable entry has an outdated timestamp, the file has been updated
    // so need broadcast and update the file
    else if( (iter->timestamp) < (entry -> timestamp) ) {
      filetable_updateFile(entry, iter, myFileTablePtr->filetable_mutex);
    }

    // if the timestamps are the same, the file is updated at the peer so add to ip list for file
    // if the ip is already in the list for the file, do not broadcast again
    else if( (iter -> timestamp) == (entry -> timestamp) ) {
      filetable_AddIp2Iplist(entry, pkt_recv.peer_ip, myFileTablePtr->filetable_mutex);
    }

    iter = iter -> next;
  }

  printf("Successfully updated the tracker filetable. Broadcast new table.\n");
  
  //at this time we finish sync fileTables between trakcer and server
  //begin to broadcast tracker's new fileTable
  broadcastFileTable();

  if (peer_filetable) {
    filetable_destroy(peer_filetable);
  }

  return 1;
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
 	if (svr_sd < 0) {
    printf("Error creating a socket.\n");
    exit(1);
 	 }
   printf("successfully created socket to wait for handshakes.\n");

	// //3. start a thread to minitor & accept alive message from online peers periodically
	// //remove dead peers if timeout occurs
 // 	pthread_t monitorAlive_thread;
 // 	pthread_create(&monitorAlive_thread, NULL, monitorAlive, NULL);


 	//4. register cleanup method when stop
 	signal(SIGINT, trackerStop);


	
	//5. keeps listening on the socket binded with HANDSHAKE_PORT
	while(1) {

		//receive the REGISTER pkt if any
		struct sockaddr_in client_addr;
		socklen_t length = sizeof(client_addr);
		int connfd = accept(svr_sd, (struct sockaddr*) &client_addr, &length);
		
    if (connfd < 0) {
      printf("Error accepting connection from peer.  Waiting to accept.\n");
      continue;
    }

    printf("Successfully connected to a peer. \n");
    printf("Waiting to receive a register packet from the peer.\n");

    ptp_peer_t pkt_recv;// the packet received on HANDSHAKE_PORT
		memset(&pkt_recv, 0, sizeof(ptp_peer_t));
    if (pkt_tracker_recvPkt(connfd, &pkt_recv) < 0) {
      printf("Error receiving the register packet\n");
    }

    printf("Successfully received the register packet.\n");


		if(pkt_recv.type == REGISTER) {

      printf("Add new peer to the peerTable\n");
			//create a new peerEntry using 1. REGISTER's ip, 2. connfd: denoting the TCP connection between this peer and tracker;
      // insert the new peerEntry into table (assert: no peer has same ip as this new peer before insertion)
      // TODO handle the case when the peer disconnects and has to reconnect
      // assert(peertable_searchEntryByIp(myPeerTablePtr, peerEntry -> ip) == NULL);
			peerEntry_t* peerEntry = peertable_createEntry(pkt_recv.peer_ip, connfd);
      peertable_addEntry(myPeerTablePtr, peerEntry);
      peertable_printPeerTable(myPeerTablePtr);
      printf("Peer table updated successfully.\n");

      printf("Try to send response to the peer with heartbeat and piece len.\n");
			//create a pkt to send back to peer, for peer to set up itself
			//the pkt contains info: 1. HEATBEAT_INTERVAL 2. PIECE_LENGTH 3. trakcer's fileTable(including size and the linkedlist)
			ptp_tracker_t* setup = pkt_create_trackerPkt();
			pkt_config_trackerPkt(setup, HEARTBEAT_INTERVAL, PIECE_LENGTH, 0, NULL);

			//send the configured pkt --> peer
			if (pkt_tracker_sendPkt(connfd, setup, myFileTablePtr -> filetable_mutex) < 0) {
        printf("Error sending the register packet\n");
        free(setup);
        continue;
      }
			
      printf("Successfully sent the packet.\n");

      //if fail the initial sync with the peer, exit the loop and terminate the 
      if (initial_sync_with_peer(connfd) < 0) {
        printf("Error Syncing With Peer. Killing connection.\n");
        free(setup);
        close(connfd);
        continue;
      }

      printf("Successfully sync the local peer. Beginning handshake thread.\n");

      //create a handshake thread to handle messages from this particular peer
      //receive messages from this peer and respond if needed
      //by using the peer-tracker handshake protocal
      printf("Initialize the handshake thread.\n");
      pthread_t handshake_thread;
      pthread_create(&handshake_thread, NULL, handshake, &connfd);

		}
	}
}










