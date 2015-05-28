#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pieceList.h"
#include "../common/constants.h"



//TODO: added mutex lock and unlock to all the operations

int PL_addToLast(pieceList_t* list, unsigned int startindex, int pieceSize){

	//create and configure
	pieceEntry_t* entry = (pieceEntry_t*)malloc(sizeof(pieceEntry_t));
	memset(entry, 0, sizeof(pieceEntry_t));

	entry->piecelen = pieceSize;
	entry->startindex = startindex;
	entry->next = NULL;

	//append
	if(list->size == 0){
		list->head = entry;
		list->tail = entry;
	} else {
		list->tail->next = entry;
		list->tail = list->tail->next;
	}
	list->size++;
	return 1;

}




/**
 * initialize the list of Need-To-Download pieces (denoted by startIndex), according to the given filesize
 * unit (everything related to filesize, are in Bytes)
 */
pieceList_t* PL_initList(unsigned int filesize){
	pieceList_t* myList = (pieceList_t*)malloc(sizeof(pieceList_t));
	memset(myList, 0, sizeof(pieceList_t));

	int lastPieceSize = PIECE_LENGTH; // if no remainder, then lastPieceSize = PIECE_LENTH
	unsigned int totalPieces = filesize / PIECE_LENGTH;
	if(filesize % PIECE_LENGTH > 0) {
		totalPieces++;
		lastPieceSize = filesize - (totalPieces - 1) * PIECE_LENGTH; // if has remainder, lastPieceSize = remiander
	}
	unsigned int i;
	for(i = 0; i < totalPieces - 1; i++){
		PL_addToLast(myList, i * PIECE_LENGTH, PIECE_LENGTH); // add the first totalPieces - 1
	}

	PL_addToLast(myList, (totalPieces - 1) * PIECE_LENGTH, lastPieceSize); // add last one


	return myList;
}





/**
 * get the next piece to be downloaded, if empty list then return NULL;
 */
pieceEntry_t* PL_getFirst(pieceList_t* list){
	if(list->size == 0) return NULL;
	pieceEntry_t* res = list->head;
	list->head = list->head->next;
	list->size --;
	return res;
}




void PL_destroy(pieceList_t* list){
	if(list->size > 0){

		pieceEntry_t* iter = list->head;
		while(iter){
			pieceEntry_t* tobeFreed = iter;
			iter = iter->next;
			free(tobeFreed);
		}
	}

	free(list);
	return;
}


