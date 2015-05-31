#ifndef PEER_H
#define PEER_H

/*#include "peer.h"
#include "file_monitor.h"
#include "../common/constants.h"
#include "../common/pkt.h"*/
#include "../common/filetable.h"
/*#include "../common/peertable.h"
#include "peer_helpers.h"
#include "../fileMonitor/fileMonitor.h"*/

int connect_to_tracker();

void* tracker_listening(void* arg);

void* p2p_listening(void* arg);

void* p2p_download(void* arg);

void* p2p_upload(void* arg);

void* file_monitor_thread(void* arg);

void* keep_alive(void* arg);

unsigned int getFilesize(const char* filename);

int setupInitialFileTable();
/*
*Sends the peers filetable to the tracker
*/
void Peer_sendfiletable();
/*
* Creates a fileEntry_t from a file name
*
*@name: name to return
*/
fileEntry_t* FileEntry_create(char* name);
/* 
* Callback add method for the File Monitor
*@name: name of the file to add
*/
void Filetable_peerAdd(char* name);
/* 
* Callback update method for the File Monitor
*@name: name of the file to update
*/
void Filetable_peerModify(char* name);
/* 
* Callback delete method for the File Monitor
*@name: name of the file to delete
*/
void Filetable_peerDelete(char* name);
/* 
* Callback sync method for the File Monitor
*@name: name of the file to add
*/
//void Filetable_peerSync(char* name);


#endif
