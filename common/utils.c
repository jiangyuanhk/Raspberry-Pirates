

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>


/**
 * get the IP from Hostname, return 1 if success, return -1 if fails
 * @param  hostname hostname from where we want get ip
 * @param  ip       the ip get from hostname
 * @return          1 if success, -1 if fails
 */
int utils_getIPfromHostName(char* hostname, char *ip) {
	struct hostent *host;
    struct in_addr **addr_list;

	host = gethostbyname(hostname);
	if(!host)
		return -1;

	addr_list = (struct in_addr **) host->h_addr_list;
	int i;
	for(i = 0; addr_list[i] != NULL; i++) {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 1; // succcess
    }
    return -1;
}



unsigned long getCurrentTime(){
	time_t curTime;
    curTime = time((time_t *)0);
    return (unsigned long) curTime;
}



