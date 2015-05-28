#include <pthread.h>

#include "../common/constants.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../common/filetable.h"
#include "../common/peertable.h"

/**** this is the list of providers for a specific peer ******/



typedef struct provider{
	char ip[IP_LEN];        // ip of the provider (peer), @ONLY_info stored in this entry
	struct provider* next;    // next provider in the list
}provider_t;




typedef struct providerList{
	provider* head;
	provider* tail;
	int size;
	pthread_mutex_t* mutex;
}providerList_t;



provider_t* PL_createProvider(char* ip);

providerList_t* PL_init();

int PL_addProvider(providerList_t* mylist, char* sourceIP);

int PL_deleteProviderByIP(providerList_t* mylist, char* sourceIP);

provider_t* getNextAvailableProvider( fileTable_t* filetable, providerList_t* providerlist, char* fileName);


int PL_destroy(providerList_t* mylist);
