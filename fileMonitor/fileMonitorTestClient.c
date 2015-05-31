#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "fileMonitor.h"


void printAdd(char* name) {
	printf("File %s successfully added\n", name);
}

void printModify(char* name) {
	printf("File %s successfully modified\n", name);
}

void printDelete(char* name) {
	printf("File %s successfully deleted\n", name);
}

void printChange() {
	printf("Change detected in file system\n");
}



int main(int argc, char *argv[]) {

	void (*Add)(char *);
	void (*Modify)(char *);
	void (*Delete)(char *);
	void (*filesChanged)(void);

	Add = &printAdd;
	Modify = &printModify;
	Delete = &printDelete;
	filesChanged = &printChange;

	/*Add = (void *)printAdd;
	Modify = (void *)printModify;
	Delete = (void *)printDelete;*/

localFileAlerts myFuncs = {
	Add,
	Modify,
	Delete,
	filesChanged
};


pthread_t monitorthread;
pthread_create(&monitorthread, NULL, fileMonitorThread, (void*) &myFuncs);

sleep(15);

blockFileAddListening("Blocked");
printf("File Blocked will be ignored if added for 15 seconds\n");

sleep(15);

unblockFileAddListening("Blocked");
blockFileWriteListening("Blocked");
printf("File Blocked will be ignored if modified for 15 seconds\n");

sleep(15);

unblockFileWriteListening("Blocked");
blockFileDeleteListening("Blocked");
printf("File Blocked will be ignored if deleted for 15 seconds\n");

sleep(15);

unblockFileDeleteListening("Blocked");

FileMonitor_close();

pthread_exit(&monitorthread);

return 1;

}
