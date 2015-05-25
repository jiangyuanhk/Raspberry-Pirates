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
	printf("File %s added\n", name);
}

void printModify(char* name) {
	printf("File %s modified\n", name);
}

void printDelete(char* name) {
	printf("File %s deleted\n", name);
}



int main(int argc, char *argv[]) {

struct localFileAlerts myFuncs {
	printAdd
	printModify
	printDelete
};


pthread_t monitorthread;
pthread_create(&monitorthread, NULL, fileMonitorThread, (void*) myFuncs);

sleep(60);

pthread_destroy(monitorthread);

}