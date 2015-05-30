
#include "common/constants.h"
#include <pthread.h>



typedef struct downloadPiece {
  char ip[IP_LEN];                      // IP address from which the piece was downloaded
  int sockfd;                           // the TCP connection to the remote peer
  
  char file_name[FILE_NAME_MAX_LEN];    // the name of the piece (.temp file)
  struct downloadPiece *next;           // the next piece in the piece_list
} downloadPiece_t;

typedef struct downloadEntry {
  char file_name[FILE_NAME_MAX_LEN];  // filepath of the local file being downloaded
  pthread_mutex_t* entry_mutex;       // mutex for the linked list of pieces
  int expected_num_pieces;            // the number of pieces the file is broken into
  downloadPiece_t* head;              // the head of the piece linked list
  downloadPiece_t* tail;              // the tail of the piece linked list
  int size;                           // the size of the linked list of pieces
  struct downloadEntry *next;         // the next download entry in the download entry list
} downloadEntry_t;

typedef struct peer_downloadTable {
    int size;                               // num of files currently in the download table
    downloadEntry_t* head;                  // head of the linked list of download entries
    downloadEntry_t* tail;                  // tail of the linked list of download entries
    pthread_mutex_t* download_table_mutex;  // mutex lock for the download table
}downloadTable_t;


//Pseudo Code

