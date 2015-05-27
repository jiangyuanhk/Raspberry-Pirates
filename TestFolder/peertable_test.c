//File: filetable_test.c

//Description: File that unit tests the functions in filetable.c.

//To compile:
// gcc -Wall -pedantic -std=c99 -ggdb -pthread -o test filetable_test.c ../common/filetable.c ../common/utils

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>

#include <time.h>
#include <assert.h>

#include "../common/peertable.h"
#include "../common/utils.h"



/*
  Function to make a mock peertable entry given an ip address and sockfd
*/

peerEntry_t* create_mock_peer_entry(char* ip, int sockfd){
  // Allocate memory for the entry.
  peerEntry_t* peerEntry = (peerEntry_t*)malloc(sizeof(peerEntry_t));

  // Set initial fields for the entry.
  memcpy(peerEntry->ip, ip, IP_LEN);
  peerEntry->sockfd = sockfd;
  peerEntry->timestamp = getCurrentTime();
  peerEntry->next = NULL;

  return peerEntry;
}

/*
Function that creates a mock peer table. The mock file table has 
size 3 with files test1.txt -> test2.txt -> test3.txt as the mock file 
entries.  test1.txt is the head and test3.txt is the tail.
*/
peerTable_t* create_mock_peertable() {
	peerTable_t* peertable = (peerTable_t*) malloc(sizeof(peerTable_t));
	peertable -> head = NULL;
	peertable -> tail = NULL;
	peertable -> size = 0;

  //add the mock test files
  peertable -> head = create_mock_peer_entry("192.123.342.212", 2);
  peertable -> head -> pNext = create_mock_peer_entry("127.000.000.001", 3);
  peertable -> head -> pNext -> pNext = create_mock_peer_entry("123.456.789.111", 4);
  peertable -> tail = peertable -> head -> pNext -> pNext;
  peertable -> size = 3;
  return peertable;
}


void test_peertable_init() {
	printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
	printf("Function: %s\n", "peertable_init");


	fileTable_t* ft = peertable_init();
	assert(ft -> head == NULL);
	assert(ft -> tail == NULL);
	assert(ft -> size == 0);
	printf("SUCCESS!!\n");

  //TODO free the table

}

void test_filetable_searchFileByName() {
	printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
	printf("Function: %s\n", "filetable_searchFileByName");

  fileTable_t* filetable = createMockFileTable();
	
  //Test entry in the table
  assert(filetable_searchFileByName(filetable -> head, "test1.txt") != NULL);
  assert(filetable_searchFileByName(filetable -> head, "test2.txt") != NULL);
  assert(filetable_searchFileByName(filetable -> head, "test3.txt") != NULL);
	printf("Successfully checks files in the table.\n");
  
  //Test entry not in the table
  assert(filetable_searchFileByName(filetable -> head, "") == NULL);
  assert(filetable_searchFileByName(filetable -> head, "test4.txt") == NULL);
  assert(filetable_searchFileByName(filetable -> head, "test.txt") == NULL);


  //TODO free the table
  printf("Successfully checks files not in the table.\n");
  printf("SUCCESS\n");
}

void test_filetable_deleteFileEntryByName(){
  printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
  printf("Function: %s\n", "filetable_deleteFileEntryByName");

  fileTable_t* filetable = createMockFileTable();
  fileEntry_t * file;

  //Delete the head and check that new head updated
  assert(filetable -> size == 3);
  file = filetable_searchFileByName(filetable -> head, "test1.txt");
  assert(file != NULL);
  assert(filetable -> head == file);
  assert(filetable_deleteFileEntryByName(filetable, "test1.txt") == 1);
  assert(filetable_searchFileByName(filetable -> head, "test1.txt") == NULL);
  file = filetable_searchFileByName(filetable -> head, "test2.txt");
  assert(filetable -> head == file);
  assert(filetable -> size == 2);
  //TODO free the old table
  printf("Successfully deleted the head from the file table.\n");


  //Delete the tail of the list and check the tail is updated
  filetable = createMockFileTable();
  assert(filetable -> size == 3);
  file = filetable_searchFileByName(filetable -> head, "test3.txt");
  assert(file != NULL);
  assert(filetable -> tail == file);
  assert(filetable_deleteFileEntryByName(filetable, "test3.txt") == 1);
  assert(filetable_searchFileByName(filetable -> head, "test3.txt") == NULL);
  file = filetable_searchFileByName(filetable -> head, "test2.txt");
  assert(filetable -> tail == file);
  assert(filetable -> size == 2);
  //TODO free the old table
  printf("Successfully deleted the tail from the file table\n");

  //Delete something from the middle of the list
  filetable = createMockFileTable();
  assert(filetable -> size == 3);
  file = filetable_searchFileByName(filetable -> head, "test2.txt");
  assert(file != NULL);
  assert(filetable -> head -> pNext == file);
  assert(filetable_deleteFileEntryByName(filetable, "test2.txt") == 1);
  assert(filetable_searchFileByName(filetable -> head, "test2.txt") == NULL);
  file = filetable_searchFileByName(filetable -> head, "test3.txt");
  assert(filetable -> head -> pNext == file);
  assert(filetable -> tail == file);
  assert(filetable -> size == 2);

  //TODO free the old table
  printf("Successfully deleted a middle file from the file table\n");
  
  printf("SUCCESS\n");
}

void test_peertable_addEntry() {
  printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
  printf("Function: %s\n", "peertable_addEntry");

  //create empty filetable
  peerTable_t* peertable = (peerTable_t*) malloc(sizeof(peerTable_t));
  peertable -> head = NULL;
  peertable -> tail = NULL;
  peertable -> size = 0;

  //Test add files to an empty table
  assert(peertable -> head == NULL);
  assert(peertable -> tail == NULL);
  assert(peertable -> size == 0);
  peerEntry_t* entry1 = create_mock_peer_entry("999.999.999.999", 8);
  peertable_addEntry(peertable, entry1);
  assert(peertable -> head == entry1);
  assert(peertable -> tail == entry1);
  assert(peertable -> size == 1);

  peerEntry_t* entry2 = create_mock_peer_entry("222.222.111.222", 4);
  peertable_addEntry(peertable, entry2);
  assert(peertable -> head == entry1);
  assert(peertable -> tail == entry2);
  assert(peertable -> head -> pNext == entry2);
  assert(peertable -> size == 2);
  printf("Successfully added files to empty table.\n");

  peerTable_t* peertable1 = create_mock_peertable();
  assert(peertable1 -> size == 3);
  peerEntry_t* entry3 = create_mock_peer_entry("234.124.111.222", 5);
  peertable_addEntry(peertable1, entry3);
  assert(peertable1 -> tail == entry3);
  assert(peertable1 -> head -> pNext -> pNext -> pNext == entry3);
  assert(peertable1 -> size == 4);
  printf("Successfully added files to table with files already in it.\n");

  //TODO - Free the necessary memory
  printf("SUCCESS\n");
}

void test_filetable_updateFile() {
  printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
  printf("Function: %s\n", "filetable_updateFile");

  fileTable_t* filetable = createMockFileTable();
  sleep(1); //allow for timestamps to change

  //Check successful update file
  fileEntry_t* old_file = filetable_searchFileByName(filetable -> head, "test3.txt");
  fileEntry_t* updated_file = create_mock_file_entry("test3.txt", 45940);

  assert(old_file != NULL);
  assert(updated_file != NULL);

  assert(old_file -> size != updated_file -> size);
  assert(old_file -> timestamp != updated_file -> timestamp);
  assert(filetable_updateFile(old_file, updated_file, filetable -> filetable_mutex) == 1);
  assert(old_file -> size == updated_file -> size);
  assert(old_file -> timestamp == updated_file -> timestamp);
  printf("Successfully updated a file.\n");
  //TODO - free the updated_file

  //Check unsuccessful update file
  old_file = filetable_searchFileByName(filetable -> head, "test2.txt");
  updated_file = create_mock_file_entry("test5.txt", 45940);
  assert(old_file -> size != updated_file -> size);
  assert(old_file -> timestamp != updated_file -> timestamp);
  assert(filetable_updateFile(old_file, updated_file, filetable -> filetable_mutex) == -1);
  assert(old_file -> size != updated_file -> size);
  assert(old_file -> timestamp != updated_file -> timestamp);
  // TODO - free the necessary memory
  printf("Successfully did not update a file when the names do not match.\n");

  printf("SUCCESS\n");
}


//Main function to test all of the functions for the peer table.
int main() {
	test_peertable_init();
  test_filetable_searchFileByName();
  test_filetable_deleteFileEntryByName();
  test_peertable_addEntry()
  test_filetable_updateFile();

  //DO NOT TEST
  //filetable_printFileTable(fileTable_t* tablePtr)

}
