#ifndef PEER_H
#define PEER_H

#include "../common/constants.h"

int connect_to_tracker();

void* tracker_listening(void* arg);

void* p2p_listening(void* arg);

void* p2p_download(void* arg);

void* p2p_upload(void* arg);

void* file_monitor_thread(void* arg);

void* keep_alive(void* arg);

unsigned int getFilesize(const char* filename);

int setupInitialFileTable();


#endif
