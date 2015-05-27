//File: filetable_test.c

//Description: File that unit tests the functions in filetable.c.

//To compile:
// gcc -Wall -pedantic -std=c99 -ggdb -pthread -o test filetable_test.c ../common/filetable.c

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

#include "../common/filetable.h"




//Function to create  amock file entry based on the given filename and size.
fileEntry_t* create_mock_file_entry(char* filename, int size){
	fileEntry_t* entry = malloc(sizeof(fileEntry_t));
	entry -> size = size;
  char*b = malloc(strlen(filename)+1);
  strcpy(b, filename);
  entry -> name = b;
  entry -> timestamp = (unsigned)time(NULL);
  entry -> pNext = NULL;
  return entry;
}

/*
Function that creates a mock file table. The mock file table has 
size 3 with files test1.txt -> test2.txt -> test3.txt as the mock file 
entries.  test1.txt is the head and test3.txt is the tail.
*/

fileTable_t* createMockFileTable() {
	fileTable_t* tablePtr = (fileTable_t*) malloc(sizeof(fileTable_t));
	tablePtr -> head = NULL;
	tablePtr -> tail = NULL;
	tablePtr -> size = 0;

	//create the mutex for the table
	pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
	tablePtr -> filetable_mutex = mutex;

  //add the mock test files
  tablePtr -> head = create_mock_file_entry("test1.txt", 12345);
  tablePtr -> head -> pNext = create_mock_file_entry("test2.txt", 6789);
  tablePtr -> head -> pNext -> pNext = create_mock_file_entry("test3.txt", 1111111);
  tablePtr -> tail = tablePtr -> head -> pNext -> pNext;
  tablePtr -> size = 3;
  return tablePtr;
}


void test_filetable_init() {
	printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
	printf("Function: %s\n", "filetable_init");


	fileTable_t* ft = filetable_init();
	assert(ft -> head == NULL);
	assert(ft -> tail == NULL);
	assert(ft -> size == 0);
	assert(ft -> filetable_mutex != NULL);
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

void test_filetable_appendFileEntry() {
  printf("~~~~~~~~~Testing Function~~~~~~~~~~~~\n");
  printf("Function: %s\n", "filetable_appendFileEntry");

  //create empty filetable
  fileTable_t* tablePtr = (fileTable_t*) malloc(sizeof(fileTable_t));
  tablePtr -> head = NULL;
  tablePtr -> tail = NULL;
  tablePtr -> size = 0;
  
  //create the mutex for the table
  pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  tablePtr -> filetable_mutex = mutex;

  //Test add files to an empty table
  assert(tablePtr -> head == NULL);
  assert(tablePtr -> tail == NULL);
  assert(tablePtr -> size == 0);
  fileEntry_t* file1 = create_mock_file_entry("test1.pdf", 1111);
  filetable_appendFileEntry(tablePtr, file1);
  assert(tablePtr -> head == file1);
  assert(tablePtr -> tail == file1);
  assert(tablePtr -> size == 1);

  fileEntry_t* file2 = create_mock_file_entry("test2.pdf", 2222);
  filetable_appendFileEntry(tablePtr, file2);
  assert(tablePtr -> head == file1);
  assert(tablePtr -> tail == file2);
  assert(tablePtr -> head -> pNext == file2);
  assert(tablePtr -> size == 2);
  printf("Successfully added files to empty table.\n");

  fileTable_t* filetable = createMockFileTable();
  assert(filetable -> size == 3);
  fileEntry_t* file3 = create_mock_file_entry("test3.pdf", 3333);
  filetable_appendFileEntry(filetable, file3);
  assert(filetable -> tail == file3);
  assert(filetable -> head -> pNext -> pNext -> pNext == file3);
  assert(filetable -> size == 4);
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
	test_filetable_init();
  test_filetable_searchFileByName();
  test_filetable_deleteFileEntryByName();
  test_filetable_appendFileEntry();
  test_filetable_updateFile();

  //DO NOT TEST
  //filetable_printFileTable(fileTable_t* tablePtr)

}
