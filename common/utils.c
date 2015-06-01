

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

#include "constants.h"

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

unsigned int getCurrentTime(){
	time_t curTime;
    curTime = time((time_t *)0);
    return (unsigned int) curTime;
}

// Function that gets the ip address for the local machine and saves it in the char * passed as a parameter.
// Parameters: char* ip_address   -> char pointer where the ip address will be memcpy'ed to 
// Returns: 1 if success, -1 if fails.
int get_my_ip(char* ip_address) {
  char hostname[MAX_HOSTNAME_SIZE];   
  gethostname(hostname, MAX_HOSTNAME_SIZE); 
  
  struct hostent *host;
  if ( (host = gethostbyname(hostname)) == NULL){
    printf("Error! Could not get the ip address from the hostname.\n");
    return -1;
  }

  //extract the ip address as a string from the hostent struct and memcpy into parameter
  char* ip = inet_ntoa( *(struct in_addr *) host -> h_addr);
  memcpy(ip_address, ip, IP_LEN);

  return 1;
}



