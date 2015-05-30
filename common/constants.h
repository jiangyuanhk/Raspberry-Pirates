#ifndef CONSTANTS_H
#define CONSTANTS_H

#define IP_LEN 20
// #define MAX_FILE_NUM 1000
#define FILE_NAME_MAX_LEN 127
#define MAX_NUM_PEERS 10
#define MAX_FILEPATH_LEN 300


#define HEARTBEAT_INTERVAL 30 // in seconds
#define PIECE_LENGTH 999 // TODO: define it
#define HANDSHAKE_PORT 3409

//Tracker Constants
#define MONITOR_ALIVE_INTERVAL 1000			//TODO
#define DEAD_PEER_TIMEOUT 1000				//TODO

//Peer Constants
#define P2P_PORT 3456
#define MAX_HOSTNAME_SIZE 128
#define PROTOCOL_LEN 1400
#define RESERVED_LEN 8


//File Monitor
#define MONITOR_POLL_INTERVAL 1


#endif
