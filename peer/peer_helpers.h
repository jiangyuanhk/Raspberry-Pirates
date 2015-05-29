#ifndef PEER_HELPERS_H
#define PEER_HELPERS_H

#include "../common/constants.h"

//Struct used in helping peer to peer file transfer.  Initially sent to
//the receiving peer before receviing any other information. 
typedef struct file_metadata{
char filename[100];
int size;                   //how large the file/ part you are sending is
int start;                  //the location of the first byte of data for the file
} file_metadata_t;


int get_my_ip(char* ip_address);

int send_register_packet(int tracker_conn);

int get_file_size(char* filepath);

file_metadata_t* send_meta_data_info(int peer_tracker_conn, char* filepath, int start, int size);

int receive_meta_data_info(int peer_tracker_conn, file_metadata_t* metadata);

int receive_data_p2p(int peer_tracker_conn, file_metadata_t* metadata);

int send_data_p2p(int peer_conn, file_metadata_t* metadata);


#endif
