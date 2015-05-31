#ifndef P2P_H
#define P2P_H

#include "../common/constants.h"
#include "../common/filetable.h"
#include <pthread.h>


typedef struct downloadPiece {
  char ip[IP_LEN];                      // IP address from which the piece was downloaded
  int sockfd;                           // the TCP connection to the remote peer
  int start;                            // the start position in the file that downloading
  int size;                             // the size of the piece to get
  int piece_num;                        // the number of the piece to include in temp file
  struct downloadPiece *next;           // the next piece in the piece_list
} downloadPiece_t;

typedef struct downloadEntry {
  char file_name[FILE_NAME_MAX_LEN];  // filepath of the local file being downloaded
  int num_pieces;                     // the number of pieces the file is broken into
  downloadPiece_t* head;              // the head of the piece linked list
  downloadPiece_t* tail;              // the tail of the piece linked list
  int successful_pieces;              // the number of pieces succcesfully sent
  int size;                           // the size of the linked list of pieces
  struct downloadEntry *next;         // the next download entry in the download entry list
  pthread_mutex_t* mutex;       // mutex for the linked list of pieces
} downloadEntry_t;

typedef struct peer_downloadTable {
    int size;                               // num of files currently in the download table
    downloadEntry_t* head;                  // head of the linked list of download entries
    downloadEntry_t* tail;                  // tail of the linked list of download entries
    pthread_mutex_t* mutex;                 // mutex lock for the download table
}downloadTable_t;

typedef struct argStruct {
    downloadEntry_t* download_entry;        // the download entry for the file
    char ip[IP_LEN];
}arg_struct_t;


downloadTable_t* init_downloadTable();

downloadEntry_t* init_downloadEntry(fileEntry_t* file, int piece_len);

downloadPiece_t* init_downloadPiece(int start, int size, int piece_num);

void append_piece_to_list(downloadEntry_t* download_entry, downloadPiece_t* piece);

downloadPiece_t* get_downloadPiece(downloadEntry_t* download_entry);

void readd_piece_to_list(downloadEntry_t* entry, downloadPiece_t* piece);

arg_struct_t* create_arg_struct(downloadEntry_t* download_entry, char* ip);



#endif



