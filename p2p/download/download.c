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

#include "../../common/constants.h"
#include "../../common/filetable.h"
#include "../../peer/peer_helpers.h"
#include "../p2p.h"


//Function that downloads p2p pieces until there are no more for the file.
void* p2p_download(void* arg) {
  
  //Read in the information from the struct and make local variables, freeing the struct
  arg_struct_t* args = (arg_struct_t*) arg;
  downloadEntry_t* entry = args -> download_entry;
  char ip[IP_LEN];
  memset(ip, 0, IP_LEN);
  memcpy(ip, args -> ip, strlen(args->ip) );
  free(args);
  
  // Create a socket and connect to the given IP address.
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(ip);      
  servaddr.sin_port = htons(P2P_PORT);

  int peer_conn = socket(AF_INET, SOCK_STREAM, 0);  

  if(peer_conn < 0) {
    printf("Error creating socket in p2p download.\n");
    pthread_exit(NULL);
  }

  if( connect(peer_conn, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    printf("Failed to connect in P2P process.\n");
    pthread_exit(NULL);
  }

  printf("Connected to a Upload Thread for IP: %s\n", ip);

  //keep polling the entry list trying to get an entry to receive.
  while(entry -> successful_pieces != entry -> num_pieces) {
    downloadPiece_t* piece = get_downloadPiece(entry);
    printf("Piece.... Start : %d Size: %d Piece Num: %d \n", piece -> start, piece -> size, piece -> piece_num);
    file_metadata_t* metadata = send_meta_data_info(peer_conn, entry -> file_name, piece -> start, piece -> size, piece -> piece_num);

    int ret2 = receive_data_p2p(peer_conn, metadata);
    printf("Ret: %d \n", ret2);
    free(metadata);
    printf("Free worked.\n");
    entry -> successful_pieces ++;
    printf("Finisiedh loop\n");
  }

  //close the connection and exit the thread
  //free any necessary memory
  close(peer_conn);
  pthread_exit(0);
}

void* p2p_upload(void* arg) {
  pthread_exit(0);
}


int main() {

  downloadTable_t* downloadtable = init_downloadTable();

  // struct sockaddr_in servaddr;
  // servaddr.sin_family = AF_INET;
  // servaddr.sin_addr.s_addr = inet_addr("129.170.214.213");      //connect to spruce
  // servaddr.sin_port = htons(P2P_PORT);

  // int peer_conn = socket(AF_INET, SOCK_STREAM, 0);  

  // if(peer_conn < 0) {
  //   printf("Error creating socket in p2p download.\n");
  //   pthread_exit(NULL);
  // }

  // if( connect(peer_conn, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
  //   printf("Failed to connect ot local ON process.\n");
  //   pthread_exit(NULL);
  // }

  // printf("Connected to a peer upload thread.\n");

  // printf("Send the file name to the peer.\n");
  // file_metadata_t* meta_info  = send_meta_data_info(peer_conn, "test.txt", 0, 53046, 0);
  // free(meta_info);  
  fileEntry_t* file = malloc(sizeof(fileEntry_t));
  file -> size = 46786572;

  memset(file -> file_name, 0, FILE_NAME_MAX_LEN);
  memcpy(file -> file_name, "test.txt", strlen("test.txt"));

  memset(file-> iplist[0], 0, IP_LEN);
  memcpy(file -> iplist[0], "129.170.214.213", strlen("129.170.214.213"));


  // memset(file-> iplist[0], 0, IP_LEN);
  // memcpy(file -> iplist[0], "129.170.213.111", strlen("129.170.213.111"));


  file -> peerNum = 1;

  downloadEntry_t* entry = init_downloadEntry(file, 5000);


  int i = 0;
  while (file -> iplist[i] ) {

  arg_struct_t* args = create_arg_struct(entry, file -> iplist[i]);
  
  pthread_t p2p_download_thread;
  pthread_create(&p2p_download_thread, NULL, p2p_download, args);


    // printf("Starting Thread for Downloading From Ip: %s\n", file -> iplist[i]);

    // while(entry -> successful_pieces != entry -> num_pieces) {
    //   downloadPiece_t* piece = get_downloadPiece(entry);
    //   printf("Piece.... Start : %d Size: %d Piece Num: %d \n", piece -> start, piece -> size, piece -> piece_num);
    //   file_metadata_t* metadata = send_meta_data_info(peer_conn, entry -> file_name, piece -> start, piece -> size, piece -> piece_num);

    //   int ret2 = receive_data_p2p(peer_conn, metadata);
    //   printf("Ret: %d \n", ret2);
    //   free(metadata);
    //   printf("Free worked.\n");
    //   entry -> successful_pieces ++;
    //   printf("Finisiedh loop\n");
    // }
    break;
    i++;
  }

  while(entry -> successful_pieces != entry -> num_pieces){
    printf("Waiting for pieces to download.\n");
    sleep(1);
  }
 
  FILE* main_file = fopen(entry -> file_name , "w");

  if (main_file == NULL) {
    printf("Error opening main file to write to.\n");
    exit(1);
  }

  int j;
  //open the new file to write
  for(j = 1; j <= entry -> num_pieces; j++) {

      char temp_filepath[300];
      sprintf(temp_filepath, "%s.temp%d", entry -> file_name, j);
      FILE* temp_file = fopen(temp_filepath, "r");
      printf("Copying File into main file: %s\n", temp_filepath);

      if (temp_file == NULL) {
        printf("Error opening temp file to write to.\n");
        exit(1);
      }

      char buffer[1000];
      size_t bytes;

      while (0 < (bytes = fread(buffer, 1, sizeof(buffer), temp_file)) ){
        fwrite(buffer, 1, bytes, main_file);
      }

      fclose(temp_file);
      remove(temp_filepath);
  }

  fclose(main_file);
  printf("Main file updated.\n");


  //loop through all of the pieces and write to the main file.

  // close(peer_conn);
}

