//FILE: peer/peer.c
//
//Description: This file implements a peer in the DartSync project.
//
//Date: May 22, 2015


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
file_metadata_t* send_meta_data_info(int peer_conn, char* filepath, int start, int size, int piece_num){
  file_metadata_t* metadata = calloc(1, sizeof(file_metadata_t));
  
  memcpy(metadata -> filename, filepath, strlen(filepath) + 1);
  metadata -> size = size;
  metadata -> start = start;
  metadata -> piece_num = piece_num;

  if (send(peer_conn, metadata, sizeof(file_metadata_t), 0) < 0) {
    printf("Failed to send meta data info.\n");
    free(metadata);
    return NULL;
  }

  return metadata;
}

// Function that recevies a file_metadata_t struct from a peer.
int receive_meta_data_info(int peer_conn, file_metadata_t* metadata) {
int num;
if ( (num = recv(peer_conn, metadata, sizeof(file_metadata_t), 0)) < 0) {
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

  char file_path[200];

  char int_buf[10];
  memset(int_buf, 0, 10);
  sprintf(file_path, "/tmp/%s.%d", strrchr(metadata -> filename, '/') + 1, metadata -> piece_num);
  printf("File Path outputing file to %s\n", file_path);

  
  FILE* file_pointer = fopen(file_path, "w");
  
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

  while ( left_to_recv != 0 && (recv_amount = recv(peer_tracker_conn, buffer, recv_buffer_size, 0)) > 0 ) {
    printf("Amount Read: %d\n", recv_amount);
    fwrite(buffer, sizeof(char), recv_amount, file_pointer);
    memset(buffer, 0, recv_amount);
    printf("Wrote fine\n");
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
    return -1;
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

int recombine_temp_files(char* filepath, int num_pieces) {
  
  printf("Recombining Pieces for Files: %s \n", filepath);
    
  //open the main file that will the pieces will be rewritten to
  FILE* main_file = fopen(filepath , "w");

  if (main_file == NULL) {
    printf("Error opening main file to write to.\n");
    exit(1);
  }

  int num;
  
  // For each piece, open the file and write the contents to the main file
  for(num = 1; num <= num_pieces; num++) {

    // create the filepath to the temporary file the piece is in
    char temp_filepath[FILE_NAME_MAX_LEN];
    sprintf(temp_filepath, "/tmp/%s.%d", strrchr(filepath, '/'), num);

    //open the temporary file that the piece is in
    FILE* temp_file = fopen(temp_filepath, "r");

    //if there is an error opening the piece, return -1
    if (temp_file == NULL) {
      printf("Error opening temp file to write to.\n");
      fclose(temp_file);
      fclose(main_file);
      return -1;
    }

    printf("Copying Piece Number: %d\n", num);

    char buffer[1500];
    size_t bytes;

    //loop through the file, read in data and write it to the main file
    while (0 < (bytes = fread(buffer, 1, sizeof(buffer), temp_file)) ){
      fwrite(buffer, 1, bytes, main_file);
    }

    printf("Exitied the read through file loop\n");
    //once done reading the temp file, close it and remove it
    fclose(temp_file);
    remove(temp_filepath);
  }

  fclose(main_file);
  return 1;
}

downloadTable_t* init_downloadTable() {

  downloadTable_t* downloadtable = malloc(sizeof(downloadTable_t));
  downloadtable -> size = 0;
  downloadtable -> head = NULL;
  downloadtable -> tail = NULL;

  //create the mutex for the table
  pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  downloadtable -> mutex = mutex;

  return downloadtable;
}

downloadEntry_t* init_downloadEntry(fileEntry_t* file, int piece_len) {
  downloadEntry_t* download_entry = malloc(sizeof(downloadEntry_t));
  memset(download_entry -> file_name, 0, FILE_NAME_MAX_LEN);
  memcpy(download_entry -> file_name, file -> file_name, strlen(file -> file_name) + 1);
  download_entry -> timestamp = file -> timestamp;

  int num_pieces = (file -> size % piece_len == 0) ? file -> size / piece_len : (file -> size / piece_len) + 1;
  download_entry -> num_pieces = num_pieces;

  //create the mutex for the table
  pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  download_entry -> mutex = mutex;

  printf("File size: %d  Piece Len: %d Num Pieces: %d\n", file -> size, piece_len, num_pieces);

  int left_to_send = file -> size;
  int piece_size = (piece_len < left_to_send) ?  piece_len : left_to_send;
  int start = 0;
  
  //loop through and add the pieces to the list
  int i;
  for(i = 1; i <= num_pieces; i++) {
    downloadPiece_t* piece = init_downloadPiece(start, piece_size, i);
    append_piece_to_list(download_entry, piece);
    left_to_send = left_to_send - piece_size;
    start = file -> size - left_to_send;
    piece_size = (piece_len < left_to_send) ?  piece_len : left_to_send;
  }

  return download_entry;
}

downloadPiece_t* init_downloadPiece(int start, int size, int piece_num) {
  downloadPiece_t* piece = malloc(sizeof(downloadPiece_t));

  piece -> start = start;
  piece -> size = size;
  piece -> piece_num = piece_num;
  piece -> next = NULL;

  return piece;
}

void append_piece_to_list(downloadEntry_t* download_entry, downloadPiece_t* piece) {
    pthread_mutex_lock(download_entry -> mutex);

  //if the table is empty, set the new file entry to be the head and tail
  if (download_entry -> size == 0) {
    download_entry -> head = piece;
    download_entry -> tail = piece;
  } 

  //otherwise, append the new file entry to the end of the list and make the new file the tail
  else {
    download_entry -> tail -> next = piece;
    download_entry -> tail = piece;
  }

  download_entry -> size ++;

  pthread_mutex_unlock(download_entry -> mutex);
  return;
}

downloadPiece_t* get_downloadPiece(downloadEntry_t* download_entry) {
  pthread_mutex_lock(download_entry -> mutex);
  downloadPiece_t* piece_to_download;

  if (download_entry -> size == 0) return NULL;

  if (download_entry -> size == 1) {
    piece_to_download = download_entry -> head;
    download_entry -> head = NULL;
    download_entry -> tail = NULL; 
  }

  else {
    downloadPiece_t* new_head = download_entry -> head -> next;
    piece_to_download = download_entry -> head;
    download_entry -> head = new_head;
  }

  download_entry -> size --;
  pthread_mutex_unlock(download_entry -> mutex);
  return piece_to_download;
}

void readd_piece_to_list(downloadEntry_t* download_entry, downloadPiece_t* piece) { 
  pthread_mutex_lock(download_entry -> mutex);

  if (download_entry -> size == 0) {
    download_entry -> head = piece;
    download_entry -> tail = piece;
  }

  else { 
    download_entry -> tail -> next = piece;
    download_entry -> tail =  piece;
  }

  download_entry -> size ++;
  pthread_mutex_unlock(download_entry -> mutex);
}

//Create struct to send to p2p upload thread.
arg_struct_t* create_arg_struct(downloadEntry_t* download_entry, char* ip) {
  arg_struct_t* args = malloc(sizeof(arg_struct_t));
  args -> download_entry = download_entry;
  
  memset(args -> ip, 0, IP_LEN);
  memcpy(args -> ip, ip, strlen(ip) + 1);

  return args;
}

//free everything in the table, table, linked list of entries and the linked list of pieces for each entry
void downloadtable_destroy(downloadTable_t* downloadtable) {
  pthread_mutex_lock(downloadtable -> mutex);
  if(downloadtable->size > 0){
    downloadEntry_t* iter = downloadtable->head;
    while(iter){
      downloadEntry_t* tobeDeleted = iter;
      iter = iter->next;
      free(tobeDeleted);
    }
  }
  pthread_mutex_unlock(downloadtable -> mutex);
  free(downloadtable->mutex);
  free(downloadtable);
}

//add an entry that has already been init'ed with init downloadEntry to the download table
void add_entry_to_downloadtable(downloadTable_t* downloadtable, downloadEntry_t* entry) {
  pthread_mutex_lock(downloadtable -> mutex);
  if(downloadtable->size == 0) {
    downloadtable->head = entry;
    downloadtable->tail = entry;
  } 

  else {
    downloadtable -> tail -> next = entry;
    downloadtable -> tail = entry;
  }

  downloadtable->size++;
  pthread_mutex_unlock(downloadtable -> mutex);
} 


downloadEntry_t* search_downloadtable_for_entry(downloadTable_t* downloadtable, char* filename) {
  pthread_mutex_lock(downloadtable -> mutex);
  if(downloadtable->size > 0){
    downloadEntry_t* iter = downloadtable -> head;
    
    while(iter){
      
      if(strcmp(iter->file_name, filename) == 0){
        pthread_mutex_unlock(downloadtable -> mutex);
        return iter;
      }
      iter = iter -> next;
    }
  }
  pthread_mutex_unlock(downloadtable -> mutex);
  return NULL;
}

int remove_entry_from_downloadtable(downloadTable_t* downloadtable, char* filename){
  pthread_mutex_lock(downloadtable -> mutex);
  downloadEntry_t* dummy = (downloadEntry_t*) malloc(sizeof(downloadEntry_t));
  dummy->next = downloadtable->head;

  downloadEntry_t* iter = dummy;
  while(iter -> next) {
    if(strcmp(iter -> next -> file_name, filename) == 0) {
      downloadEntry_t* tobeDeleted = iter->next;
      iter->next = iter->next->next;
      free(tobeDeleted);
      pthread_mutex_unlock(downloadtable -> mutex);
      return 1;
    }
    iter = iter -> next;
  }
  
  pthread_mutex_unlock(downloadtable -> mutex);
  return -1;
}





