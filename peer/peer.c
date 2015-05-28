//FILE: peer/peer.c
//
//Description: This file implements a peer in the DartSync project.
//
//Date: May 22, 2015

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>

#include "../common/constants.h"
#include "../common/pkt.h"
#include "../common/filetable.h"
#include "../commom/peertable.h"
#include "peer_helpers.h"



// Globals Variables
int tracker_connection;     // socket connection between peer and tracker so can send / receive between the two
int keep_alive_interval;
int piece_length;

fileTable_t* filetable;     //local file table to keep track of files in the directory
peerTable_t* peertable;     //peer table to keep track of ongoing downloading tasks


//Function to connect the peer to the tracker on the HANDSHAKE Port.
// Returns -1 if it failed to connect.  Otherwise, returns the sockfd
int connect_to_tracker() {
  int tracker_connection;
  struct sockaddr_in servaddr;
  struct hostent *hostInfo;

  //Get the host name by requesting user input and check if valid
  char hostname[MAX_HOSTNAME_SIZE];
  printf("Enter hostname of the tracker to connect to:");
  scanf("%s",hostname);

  hostInfo = gethostbyname(hostname);
  if (!hostInfo) {
    printf("Error with the hostname of the tracker!\n");
    return -1;
  }
    
  // Set up the tracker address info so we can connect to it
  servaddr.sin_family = hostInfo->h_addrtype;  
  memcpy((char *) &servaddr.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
  servaddr.sin_port = htons(TRACKER_PEER_PORT);

  // Create socket on local host to connect to the tracker
  tracker_connection = socket(AF_INET, SOCK_STREAM, 0);  
  if (tracker_connection < 0) {
    return -1;
  }

  printf("Attempting to connect to the tracker.\n");
  if (connect (tracker_connection, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    return -1;
  }

  printf("Successfully connected to the tracker.\n");
  return tracker_connection; 
}

//Thread to listen for messages from the tracker.  Upon receiving messages from the tracker, it looks
// to sync the local files with the tracker file knowledge, creating download threads as necessary.
void* tracker_listening(void* arg) {

  ptp_tracker_t* pkt = malloc(sizeof(ptp_tracker_t));

  //continuously receive packets from the tracker
  while(pkt_peer_recvPkt(tracker_connection, pkt) < 0) {

    //extract the file table from the packet from the tracker
    fileTable_t* master_ft = pkt -> file_table;

    //loop through the master file table to see which files need to be synchronized locally
    fileEntry_t* file = master_ft -> head;
    while (file != NULL) {

      //check to see if the file exists locally
      fileEntry_t* local_file = filetable_searchFileByName(filetable, file -> name); 
      
      // download the updated file if the local file does not exist or the local file is outdated
      if(local_file == NULL || (file -> timestamp) > (local_file -> timestamp) ) { 
        //TODO make sure that the file is not already being downloaded and if not, add to the peer table
        pthread_t p2p_download_thread;
        pthread_create(&p2p_download_thread, NULL, p2p_download, file);
      }

      file = file -> next;  //move to next item in file table from tracker
    }

    //Delete files that are in local table, but not in the file table from the tracker
    file = filetable -> head;
    while (file != NULL) {

      //check to see if the local file exists in the tracker table
      fileEntry_t* master_file = filetable_searchFileByName(master_ft, file -> name);

      //if the file is not in the master file table and is locally, then delete it
      if (master_file == NULL) {

         if( remove(file -> name) == 0) {
          printf("Successfully removed the file in filesystem: %s \n", file -> name);
          file = file -> next;
          filetable_deleteFileEntryByName(filetable, char* filename);
         }

         else{
          file = file -> next;
          printf("Error in removing the file from the file system.\n");
         }
      }
    }
  }

  free(pkt);
  pthread_exit(NULL);
}


// Thread to listen on the P2P port for download requests from other peers to spawn upload threads.
// First initializes a port to listen for requests to connect to it and then listens.
void* p2p_listening(void* arg) {
  
  //Set up the socket for listening
  int peer_sockfd;
  struct sockaddr_in local_peer_addr;

  struct sockaddr_in other_peer_addr;
  socklen_t other_peer_addr_len;

  peer_sockfd = socket(AF_INET, SOCK_STREAM, 0); 
  if (peer_sockfd < 0) {
    pthread_exit(NULL);
  }

  //Set up the local addr info
  memset(&local_peer_addr, 0, sizeof(local_peer_addr));
  local_peer_addr.sin_family = AF_INET;
  local_peer_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  local_peer_addr.sin_port = htons(P2P_PORT);

  //Attempt to bind the socket
  if (bind (peer_sockfd, (struct sockaddr *)&local_peer_addr, sizeof(local_peer_addr)) < 0){
    printf("Failed to bind.\n");
    pthread_exit(NULL);
  }

  //Attempt to set up socket for listening
  if (listen (peer_sockfd, MAX_NUM_PEERS) < 0) {
    printf("Failed to listen.\n");
    pthread_exit(NULL);
  }
  
  //Start infinite loop waiting to accept connections from peers.
  //When a peer connects, we create a upload thread to handle that connection.  
  while(1) {
    int peer_conn;

    peer_conn = accept(peer_sockfd, (struct sockaddr*) &other_peer_addr, &other_peer_addr_len);
    printf("Established a connection to a peer. Starting a P2PDownload Thread.\n");

    //start the p2p_download_thread
    pthread_t p2p_upload_thread;
    pthread_create(&p2p_upload_thread, NULL, p2p_upload, &peer_conn);
  }

  pthread_exit(NULL);
}


/* Thread to download a file from a peer.  First establishes the connecting with a peer.
   Then, sends a file_metadata_t to the peer to let it know the name of the file it needs to download.
   Next, it receives a file_metadata_t from the peer to let it know its about to receive the data and containing 
   information about the start and send_size as well as the file name.  Then, it receives the file and closes
   the connection */
void* p2p_download(void* arg) {
  fileEntry_t* file = (fileEntry_t*) arg;

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(file -> iplist[0]);      //just use the first ip in the list
  servaddr.sin_port = htons(PTP_PORT);

  int peer_conn = socket(AF_INET, SOCK_STREAM, 0);  

  if(peer_conn < 0) {
    printf("Error creating socket in p2p download.\n");
    return -1;
  }

  if( connect(peer_conn, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    printf("Failed to connect ot local ON process.\n");
    return -1;
  }

  printf("Connected to a peer upload thread.\n");

  //Download data from the peer
  //Send the name of the file you need to download
  file_metadata_t* meta_info  = send_meta_data_info(peer_conn, file -> name, 0, 0);
  free(metadata);

  //Recv the file
  file_metadata_t* metadata = calloc(1, sizeof(file_metadata_t));
  int ret1 = receive_meta_data_info(connfd, metadata);
  int ret2 = receive_data_p2p(connfd, metadata);
  printf("Ret1: %d    Ret2: %d \n", ret1, ret2);
  free(metadata);

  //TODO
  //handshake with the tracker
  //send the filetable so that I can be added to the iplist[0]

  //TODO
  //remove this from the peer table so know that the download is completed
  //close the connection

  close(peer_conn);
  pthread_exit(NULL);
}


/* Thread to upload a file to a peer. First accepts the connection from a peer.
   Then, receives a file_metadata_t from the peer to let it know the name of the file it needs to upload.
   Next, it sends a file_metadata_t to the peer to let it know its about to send the data and containing 
   information about the start and send_size as well as the file name.  Then, it sends the file and closes
   the connection */
void* p2p_upload(void* arg) {
  int peer_conn = *(int*) arg;  //gonna be casting issue here
  
  //get the filename of the file to send  
  file_metadata_t* recv_metadata = malloc(sizeof(file_metadata_t));
  receive_meta_data_info(peer_conn, recv_metadata);

  //Sending a file p2p 
  printf("Sending a File: %s \n"recv_metadata -> filename);
  file_metadata_t* metadata = send_meta_data_info(peer_conn, recv_metadata -> filename, 0, get_file_size(recv_metadata -> filename));
  send_data_p2p(tracker_connection, metadata);
  free(metadata);
  free(recv_metadata);
  close(peer_conn);
  pthread_exit(NULL);
}



/* Thread to monitor local file changes.  When a file is changed locally, inform the tracker by sending the filetable
  to the tracker.
*/
void* file_monitor_thread(void* arg){
  //TODO Intergrate the file monitor logic from Daniel's File Monitor
  pthread_exit(NULL);
}

void* keep_alive(void* arg) {
  int interval = *(int*) arg;
  while(1) {
    sleep(interval);
    //TODO
    //send the keep alive message
  }

  pthread_exit(NULL);
}

int main(){
  
  //Initialize the filetable
  fileTable_t* filetable = malloc(sizeof(fileTable_t));
  //TODO
  //load the local directory into the file table

  //Initialize the peer table
  peerTable_t* peertable = malloc(sizeof(peerTable_t)); 

  //Attempt to establish connection with tracker
  if ( (tracker_connection = connect_to_tracker()) < 0) {
    printf("Failed to connect to tracker. Exiting\n");
    exit(0);
  }

  printf("Connected\n");

  //Send a register packet to the tracker
  if (send_register_packet(tracker_connection) < 0) {
    printf("Failed to send register packet\n");
    return -1; // maybe exit();
  }

  //Receive the acknowledgement of the register packet from the tracker
  ptp_tracker_t* packet = calloc(1, sizeof(ptp_tracker_t));
  recv_pkt_from_tracker(packet, tracker_connection);
  keep_alive_interval = packet -> interval;
  printf("Receiving packet from the tracker.\n");
  printf("Inteval: %d  Piece Len: %d   FT-Size: %d \n", packet -> interval, packet -> piece_len, packet -> file_table_size);
  free(packet);

  //Start the keep alive thread
  pthread_t keep_alive_thread;
  pthread_create(&keep_alive_thread, NULL, keep_alive, keep_alive_interval);

  //start the thread to listen for data from the tracker
  pthread_t tracker_listening_thread;
  pthread_create(&tracker_listening_thread, NULL, tracker_listening, (void*)0);

  //start the thread to listen on the p2p port for connections from other peers
  pthread_t p2p_listening_thread;
  pthread_create(&p2p_listening_thread, NULL, p2p_listening, &tracker_connection);
}














/**** added by Yuan ******/



//Need
//  downloadlist -- peer's download list to keep track of all the files that are currently being downloaded
//  fileTable -- peer side file Table
//  
//  
//
//
//
//
//




/* Thread to download a file from all available peers.  First establishes the connecting with a peer.
   Then, sends a file_metadata_t to the peer to let it know the name of the file it needs to download.
   Next, it receives a file_metadata_t from the peer to let it know its about to receive the data and containing 
   information about the start and send_size as well as the file name.  Then, it receives the file and closes
   the connection */
void* p2p_download(void* arg) {
  fileEntry_t* file = (fileEntry_t*) arg;

  struct sockaddr_in servaddr;
  servaddr.sin_family = FIlAF_INET;
  servaddr.sin_addr.s_addr = inet_addr(file -> iplist[0]);      //just use the first ip in the list
  servaddr.sin_port = htons(PTP_PORT);

  int peer_conn = socket(AF_INET, SOCK_STREAM, 0);  

  if(peer_conn < 0) {
    printf("Error creating socket in p2p download.\n");
    return -1;
  }

  if( connect(peer_conn, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    printf("Failed to connect ot local ON process.\n");
    return -1;
  }

  printf("Connected to a peer upload thread.\n");


  //for this file, Need to create and manage:
  //      1. @providerList (file specific): keep track of active providers online
  //      2. @pieceList (file specific): keep track of still-need pieces




  //check if the filename is already in the @downloadFileList, if it is already being downloaded, then wait for a while and check again
  

  //if it is not yet being downloaded, then start to download it
  // 1. add the file to the @downloadlist
  // 2. create a pieceList to keep track of all the pieces needed for this particular file
  // 
         // query the fileTable to find the particular file (find by filename), fetch the iplist, and obtain one available (not in the providerList) ip (source peer) to download file from...
         // if successfully picked one available source (sourceIP in the iplist ):
         //   1. add it to the providerList
         //   2. try to start a thead managing downloading this file from a particular peer (given IP address)
         //   3. 




}





// need to know: 
// 1. @filename to be downloaded
// 2. the filesize of the file to be downloaded
// 3  IP of the provider 
// 4. master thread's: 
//              1. @providerList
//              2. @pieceList
void* p2p_ManageDownloadFileFromOnePeer(){
    // 1. parse the arg to get all the info we want
    // 2. connect to the uploader (@sourceip)
   
    
    // while(@pieceList is not empty):{
    //     1. get the next piece (peek at the head of @pieceList), this is the piece we want to download now  (Node: 许多这层thread都可以 query 同一个@pieceList, 全都是从左到右)
    //     
    //     
    //     1. request the piece from @sourceip (1.filename, 2. which piece to download )
    //     2. recv the requested piece from the @sourceip into a small buffer
    //          if SUCCESS, then write to the @tempFile (copy buffer --> @tempFile)
    //          if FAILURE, then TODO:
    //          
    //          
    //     3. remove this piece from @pieceList (update @pieceList)
    // 
    // 
    //      
    // 
    // 
    // }
    // 

}








