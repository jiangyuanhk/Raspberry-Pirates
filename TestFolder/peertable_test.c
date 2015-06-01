//File: filetable_test.c

//Description: File that unit tests the functions in filetable.c.

//To compile:
// gcc -Wall -pedantic -std=c99 -ggdb -pthread -o test filetable_test.c ../common/filetable.c ../common/utils

//To check for memory leaks after compiling:
// valgrind -v --leak-check=full ./test

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

  //create the mutex for the table
  pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  peertable -> peertable_mutex = mutex;

  //add the mock test files
  peertable -> head = create_mock_peer_entry("192.123.342.212", 2);
  peertable -> head -> next = create_mock_peer_entry("127.000.0.1", 3);
  peertable -> head -> next -> next = create_mock_peer_entry("123.456.789.92", 4);
  peertable -> tail = peertable -> head -> next -> next;
  peertable -> size = 3;
  return peertable;
}

void test_peertable_init() {
	printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
	printf("Function: %s\n", "peertable_init");

	peerTable_t* peertable = peertable_init();
	assert(peertable -> head == NULL);
	assert(peertable -> tail == NULL);
	assert(peertable -> size == 0);
  assert(peertable -> peertable_mutex != NULL);
	printf("SUCCESS!!\n");

  peertable_destroy(peertable);
  peertable = NULL;
}


void test_peertable_createEntry() {
  printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
  printf("Function: %s\n", "peertable_createEntry");

  peerEntry_t* peer = peertable_createEntry("192.123.342.212", 5);
  sleep(1);
  assert(strcmp(peer -> ip, "192.123.342.212") == 0);
  assert(peer -> sockfd == 5);
  assert(peer -> timestamp <= getCurrentTime());
  assert(peer -> next == NULL);
  printf("SUCCESS!!\n");

  free(peer);
  peer = NULL;
}

void test_peertable_searchEntryByIp() {
	printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
	printf("Function: %s\n", "peertable_searchEntryByIp");

  peerTable_t* peertable = create_mock_peertable();
	
  //Test entry in the table
  assert(peertable_searchEntryByIp(peertable, "192.123.342.212") != NULL);
  assert(peertable_searchEntryByIp(peertable, "127.000.0.1") != NULL);
  assert(peertable_searchEntryByIp(peertable, "123.456.789.92") != NULL);
	printf("Successfully checks files in the table.\n");
  
  //Test entry not in the table
  assert(peertable_searchEntryByIp(peertable, "") == NULL);
  assert(peertable_searchEntryByIp(peertable, "127.000.0.111") == NULL);
  assert(peertable_searchEntryByIp(peertable, "123.222.222.222") == NULL);
  printf("Successfully checks files not in the table.\n");

  peertable_destroy(peertable);
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

  //create the mutex for the table
  pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  peertable -> peertable_mutex = mutex;

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
  assert(peertable -> head -> next == entry2);
  assert(peertable -> size == 2);
  printf("Successfully added files to empty table.\n");

  peertable_destroy(peertable);
  peertable = NULL;

  // Test adding files to an existing table
  peertable = create_mock_peertable();
  assert(peertable -> size == 3);
  peerEntry_t* entry3 = create_mock_peer_entry("234.124.111.222", 5);
  peertable_addEntry(peertable, entry3);
  assert(peertable -> tail == entry3);
  assert(peertable -> head -> next -> next -> next == entry3);
  assert(peertable -> size == 4);
  printf("Successfully added files to table with files already in it.\n");

  peertable_destroy(peertable);

  printf("SUCCESS\n");
}

void test_peertable_deleteEntryByIp() {
  printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
  printf("Function: %s\n", "peertable_deleteEntryByIp");

  peerTable_t* peertable = create_mock_peertable();
  peerEntry_t * peer;

  //Delete the head and check that new head updated
  assert(peertable -> size == 3);
  peer = peertable_searchEntryByIp(peertable, "192.123.342.212");
  assert(peer != NULL);
  assert(peertable -> head == peer);
  assert(peertable_deleteEntryByIp(peertable, "192.123.342.212") == 1);
  assert(peertable_searchEntryByIp(peertable, "192.123.342.212") == NULL);
  peer = peertable_searchEntryByIp(peertable, "127.000.0.1");
  assert(peertable -> head == peer);
  assert(peertable -> size == 2);
  peertable_destroy(peertable);
  peertable = NULL;
  printf("Successfully deleted the head from the peer table.\n");

  //Delete the tail of the list and check the tail is updated
  peertable = create_mock_peertable();
  assert(peertable -> size == 3);
  peer = peertable_searchEntryByIp(peertable, "123.456.789.92");
  assert(peer != NULL);
  assert(peertable -> tail == peer);
  assert(peertable_deleteEntryByIp(peertable, "123.456.789.92") == 1);
  assert(peertable_searchEntryByIp(peertable, "123.456.789.92") == NULL);
  peer = peertable_searchEntryByIp(peertable, "127.000.0.1");
  assert(peertable -> tail == peer);
  assert(peertable -> size == 2);
  peertable_destroy(peertable);
  peertable = NULL;
  printf("Successfully deleted the tail from the peer table\n");

  //Delete something from the middle of the list
  peertable = create_mock_peertable();
  assert(peertable -> size == 3);
  peer = peertable_searchEntryByIp(peertable, "127.000.0.1");
  assert(peer != NULL);
  assert(peertable -> head -> next == peer);
  assert(peertable_deleteEntryByIp(peertable, "127.000.0.1") == 1);
  assert(peertable_searchEntryByIp(peertable, "127.000.0.1") == NULL);
  peer = peertable_searchEntryByIp(peertable, "123.456.789.92");
  assert(peertable -> head -> next == peer);
  assert(peertable -> tail == peer);
  assert(peertable -> size == 2);
  peertable_destroy(peertable);
  peertable = NULL;
  printf("Successfully deleted a middle peer from the peer table\n");
  
  printf("SUCCESS\n");
}

void test_peertable_refreshTimestamp() {
  printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
  printf("Function: %s\n", "peertable_refreshTimestamp");

  peerTable_t* peertable = create_mock_peertable();
  peerEntry_t* peer = peertable -> head;
  sleep(1);

  unsigned int timestamp = peer -> timestamp;

  assert(peertable_refreshTimestamp(peer) == 1);
  assert(timestamp != peer -> timestamp);
  assert(peer -> timestamp == getCurrentTime());
  peertable_destroy(peertable);
  peertable = NULL;
  printf("Successfully updated the timestamp.\n");

  printf("SUCCESS!!\n");
}

//Main function to test all of the functions for the peer table.
int main() {
	test_peertable_init();
  test_peertable_createEntry();
  test_peertable_searchEntryByIp();
  test_peertable_addEntry();
  test_peertable_deleteEntryByIp();
  test_peertable_refreshTimestamp();
}
