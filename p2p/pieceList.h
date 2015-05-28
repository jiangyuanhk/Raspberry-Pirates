#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>




/** this is to keep track all the remaining pieces for a particular file that is needed to be downloaded 
* simulate a queue, add to tail, remove from head
*/

typedef struct pieceEntry{
	unsigned int startindex; // the starting pos of this piece in the file
	int piecelen;     	// the size of this piece
	struct pieceEntry* next;
}pieceEntry_t;



typedef struct pieceList {
	pieceEntry_t* head;
	pieceEntry_t* tail;
	int size;
	pthread_mutex_t* mutex;
} pieceList_t;



pieceList_t* PL_initList(unsigned int filesize);



pieceEntry_t* PL_getFirst(pieceList_t* list);



int PL_addToLast(pieceList_t* list, unsigned int nextStartIndex, int pieceSize);


void PL_destroy(pieceList_t* list);