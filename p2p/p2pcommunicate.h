

#include "../common/constants.h"




/* 
 *	downloader need to tell the targeted uploader: 
 *	1. filename to be downloaded
 *	2. pieceID
 */
typedef struct p2p_dl_request {
  char *filename;        
  //TODO: 
} p2p_dl_request_t;


/**
 * uploader needs to tell requesting downloader:
 * 1. is download success or fail ? (if no response is sent, also fails)
 * 2. 
 */
typedef struct p2p_ul_response {
  //TODO: 
} p2p_ul_response_t;



typedef struct file_piece{
	char* filename; // name of the file
	int pieceID; // pieceID should be unique in a particular 
	char* buf;	// content of file_piece
	int size; // either a value < PIECE_LEN or PIECE_LEN
}file_piece_t;





/*** file pieces stuff *******/


/**
 * get the 
 * @return [description]
 */
char* getFilePiece(char* filename, ){
	//location: relative to root dir of local 
	
	
	
}




char* 







