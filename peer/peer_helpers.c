//FILE: peer/peer.c
//
//Description: This file implements a peer in the DartSync project.
//
//Date: May 22, 2015


#include <stdlib.h>
#include <stdio.h>
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


#include "peer_helpers.h"
#include "../common/constants.h"


#define BUFFER_SIZE 1500

// // Function that gets the ip address for the local machine and saves it in the char * passed as a parameter.
// // Parameters: char* ip_address   -> char pointer where the ip address will be memcpy'ed to 
// // Returns: 1 if success, -1 if fails.
// int get_my_ip(char* ip_address) {
//   char hostname[MAX_HOSTNAME_SIZE];   
//   gethostname(hostname, MAX_HOSTNAME_SIZE); 
  
//   struct hostent *host;
//   if ( (host = gethostbyname(hostname)) == NULL){
//     printf("Error! Could not get the ip address from the hostname.\n");
//     return -1;
//   }

//   //extract the ip address as a string from the hostent struct and memcpy into parameter
//   char* ip = inet_ntoa( *(struct in_addr *) host -> h_addr);
//   memcpy(ip_address, ip, IP_LEN);

//   return 1;
// }



// int send_keep_alive_packet(int file_table_size, int tracker_conn) { //need a table size as well
//   ptp_peer_t* packet = calloc(1, sizeof(ptp_peer_t));
//   packet -> type = KEEP_ALIVE;
//   get_my_ip(packet-> peer_ip);
//   packet -> port = P2P_PORT;
//   packet -> filetablesize = file_table_size;
//   //TODO factor in the file table that will be sent as an array
//   //peer table file table

//   if (send(tracker_conn, packet, sizeof(ptp_peer_t), 0) < 0) {
//     free(packet);
//     printf("Error sending the register packet\n");
//     return -1;
//   }

//   free(packet);
//   return 1;
// }

//Function that, given a filepath, returns the size of the file.
int get_file_size(char* filepath) {
  FILE *fp = fopen(filepath, "r");
  
  //Return -1 if unable to open the file
  if (fp == NULL) {
    return -1;
  }

  fseek(fp, 0L, SEEK_END);
  int file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  fclose(fp);
  return file_size;
}

// Function to send meta data to another peer before sending the actual file
file_metadata_t* send_meta_data_info(int peer_conn, char* filepath, int start, int size){
  file_metadata_t* metadata = calloc(1, sizeof(file_metadata_t));
  
  memcpy(metadata -> filename, filepath, strlen(filepath) + 1);
  metadata -> size = size;
  metadata -> start = start;

  if (send(peer_conn, metadata, sizeof(file_metadata_t), 0) < 0) {
    free(metadata);
    return NULL;
  }

  return metadata;
}

// Function that recevies a file_metadata_t struct from a peer.
int receive_meta_data_info(int peer_conn, file_metadata_t* metadata) {
if (recv(peer_conn, metadata, sizeof(file_metadata_t), 0) < 0) {
    return -1;
  }

  return 1;
}

/* 
  Function that receives data from a peer and updates the file.
  Input: int peer_tracker_conn - the connection to the other peer to receive data on
         file_metadata_t* metadata - metadata information on the file being sent
  Returns 1 on success, -1 on failure 
  */
int receive_data_p2p(int peer_tracker_conn, file_metadata_t* metadata){

  FILE* file_pointer = fopen(metadata -> filename , "w");
  
  if (file_pointer == NULL) {
    printf("Error opening file!\n");
    return -1;
  }

  //need to factor the edge case if not the full buffer
  //just have more logic for calcuating the size of what receiving
  char buffer[BUFFER_SIZE];
  int recv_amount;

  int left_to_recv = metadata -> size;
  int recv_buffer_size = (BUFFER_SIZE < left_to_recv) ?  BUFFER_SIZE : left_to_recv;

  while ( (recv_amount = recv(peer_tracker_conn, buffer, recv_buffer_size, 0)) > 0 ) {
    printf("Amount Room: %d\n", recv_amount);
    fwrite(buffer, sizeof(char), recv_amount, file_pointer);
    memset(buffer, 0, recv_amount);

    left_to_recv = left_to_recv - recv_amount;
    recv_buffer_size = (BUFFER_SIZE < left_to_recv) ?  BUFFER_SIZE : left_to_recv;
  }

  fclose(file_pointer);
  return 1;
}

/* Function that sends the data for a give file path from one per
   to another via a given conneciton.
   Input: int peer_conn (connection to the other peer to send data over)
          file_metadata_t* metadata -> metadata structure with file info
   Returns 1 on success, -1 on failure 
  */
int send_data_p2p(int peer_conn, file_metadata_t* metadata) {
  FILE* fp;

  fp = fopen(metadata -> filename, "r");

  if (fp == NULL) {
    printf("Failed to open the file at filepath:%s\n", metadata -> filename);
    exit(1);
  }

  //Move the file pointer to the beginnng of what is desired to be sent
  fseek(fp, metadata -> start, SEEK_SET); 
  int left_to_send = metadata -> size;
  char buffer[BUFFER_SIZE];
  size_t send_size;
  

  int send_buffer_size = (BUFFER_SIZE < left_to_send) ?  BUFFER_SIZE : left_to_send;
  while( (send_size = fread(buffer, sizeof(char), send_buffer_size, fp)) > 0) {
    printf("Send Size: %zu\n", send_size);

    if (send(peer_conn, buffer, send_size, 0) < 0){
      printf("Error Sending Data.\n");
      fclose(fp);
      return -1;
    }

    left_to_send = left_to_send - (int) send_size;
    send_buffer_size = (BUFFER_SIZE < left_to_send) ?  BUFFER_SIZE : left_to_send;
  }

  fclose(fp);
  return 1;
}




