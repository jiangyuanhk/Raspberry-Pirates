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
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <assert.h>
// #include "tracker.h"
#include "../common/pkt.h"
#include "../common/filetable.h"
#include "../common/peertable.h"
#include "../common/utils.h"


fileTable_t* ft = NULL;

//file monitor main thread
int main(int argc, char *argv[]) {

ft = filetable_init();

if(!ft) {
	printf("Filetable init failed\n");
	return -1; 
}





while(1) {

}

return NULL;



}