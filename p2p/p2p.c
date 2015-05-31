

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
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include "p2p.h"

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
}

//Create struct to send to p2p upload thread.
arg_struct_t* create_arg_struct(downloadEntry_t* download_entry, char* ip) {
  arg_struct_t* args = malloc(sizeof(arg_struct_t));
  args -> download_entry = download_entry;
  
  memset(args -> ip, 0, IP_LEN);
  memcpy(args -> ip, ip, strlen(ip) + 1);

  return args;
}

