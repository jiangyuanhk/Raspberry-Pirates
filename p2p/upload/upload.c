#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../../common/constants.h"
#include "../p2p.h"
#include "../../peer/peer_helpers.h"

// #define BUFFER_SIZE 1500


void* p2p_upload(void* arg) {

  int peer_conn = *(int*) arg;

  printf("Receiving file_metadata_t\n");
  
  //get the filename of the file to send  
  file_metadata_t* recv_metadata = malloc(sizeof(file_metadata_t));
  
  receive_meta_data_info(peer_conn, recv_metadata);

  printf("Metadata. Name: %s, Size: %d, Start: %d\n", recv_metadata -> filename, recv_metadata -> size, recv_metadata -> start);

  //Sending a file p2p 
  printf("Sending a File: %s \n", recv_metadata -> filename);
  
  send_data_p2p(peer_conn, recv_metadata);

  free(recv_metadata);

  pthread_exit(0);
}



int main() {

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
}












  // //Set up the socket for listening
  // int peer_sockfd;
  // struct sockaddr_in local_peer_addr;

  // struct sockaddr_in other_peer_addr;
  // socklen_t other_peer_addr_len;

  // peer_sockfd = socket(AF_INET, SOCK_STREAM, 0); 
  // if (peer_sockfd < 0) {
  //   pthread_exit(NULL);
  // }

  // //Set up the local addr info
  // memset(&local_peer_addr, 0, sizeof(local_peer_addr));
  // local_peer_addr.sin_family = AF_INET;
  // local_peer_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  // local_peer_addr.sin_port = htons(P2P_PORT);

  // //Attempt to bind the socket
  // if (bind (peer_sockfd, (struct sockaddr *)&local_peer_addr, sizeof(local_peer_addr)) < 0){
  //   printf("Failed to bind.\n");
  //   pthread_exit(NULL);
  // }

  // //Attempt to set up socket for listening
  // if (listen (peer_sockfd, MAX_NUM_PEERS) < 0) {
  //   printf("Failed to listen.\n");
  //   pthread_exit(NULL);
  // }
  
  
//   int peer_conn = accept(peer_sockfd, (struct sockaddr*) &other_peer_addr, &other_peer_addr_len);
//   printf("Established a connection to a peer. Starting a P2PDownload Thread.\n");

//   while(1) {

//     printf("Receiving file_metadata_t\n");
//     //get the filename of the file to send  
//     file_metadata_t* recv_metadata = malloc(sizeof(file_metadata_t));
//     receive_meta_data_info(peer_conn, recv_metadata);

//     printf("Metadata. Name: %s, Size: %d, Start: %d\n", recv_metadata -> filename, recv_metadata -> size, recv_metadata -> start);

//     //Sending a file p2p 
//     printf("Sending a File: %s \n", recv_metadata -> filename);
    
//     send_data_p2p(peer_conn, recv_metadata);

//     free(recv_metadata);
//   }
// }

  


  // //Sending a file p2p 
  // printf("Sending a File: %s \n", recv_metadata -> filename);
  // file_metadata_t* metadata = send_meta_data_info(peer_conn, recv_metadata -> filename, 0, get_file_size(recv_metadata -> filename));
  // send_data_p2p(tracker_connection, metadata);
  // free(metadata);
  // free(recv_metadata);
  // close(peer_conn);
  // pthread_exit(NULL);

