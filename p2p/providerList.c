
#include "providerlist.h"



provider_t* PL_createProvider(char* ip){
	provider_t* newprovider = (provider_t*)malloc(sizeof(provider_t));
	newprovider->next = NULL;
	strcpy(newprovider->ip, ip);
	return newprovider;
}




/* Make a new empty provider list */
providerList_t* PL_init(){
    providerList_t* mylist = (providerList_t*)malloc(sizeof(providerList_t));
    mylist -> head = NULL;
    mylist -> size = 0;
    mylist -> mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mylist->mutex, NULL);
    return mylist;
}



/**
 * create an provider identified by sourceIP, and add it to the last of the provider_list
 * return -1 if sourceIP already exist, return 1 if it is new to the providerList and add successfully
 */
int PL_addProvider(providerList_t* mylist, char* sourceIP){



	if(PL_searchProviderByIP(mylist, sourceIP)){
		return -1;//can find it, means already exist, so return -1 to say addition failed
	}

	//not eixst before adding
    provider_t* provider = malloc(sizeof(provider_t));
    provider->next = NULL;
    strcpy(provider->ip, sourceIP); // TODO: strcpy or memcpy
    //add to last
    pthread_mutex_lock(mylist->mutex);
   	if(mylist->size == 0){
   		mylist->head = provider;
   		mylist->tail = provider;
   	} else {
   		mylist->tail->next = provider;
   		mylist->tail = mylist->tail->next;
   	}
   	mylist->size++;
    pthread_mutex_unlock(mylist->mutex);
}





int PL_searchProviderByIP(providerList_t* providerlist, char* sourceIP){
	//go through the list to see if provider exist
	pthread_mutex_lock(providerlist->mutex);
	provider_t* iter = providerlist->head;
	while(iter != NULL){
		if(strcmp(iter->ip, sourceIP) == 0)
			pthread_mutex_unlock(providerlist->mutex);
			return 1;//if exist, return 1
		iter = iter->next;
	}
	pthread_mutex_unlock(providerlist->mutex);
	return -1;
}


/* Remove uploader from uploader list */
int PL_deleteProviderByIP(providerList_t* mylist, char* sourceIP){
    
	if(mylist->size == 0){
		return -1;
	}

	provider_t* dummy = (provider_t*)malloc(sizeof(provider_t));
	dummy->next = mylist->head;
	provider_t* iter = dummy;

	pthread_mutex_lock(mylist->mutex);
	while(iter->next){
		if(strcmp(iter->next->ip, sourceIP) == 0){
			//begin to delete
			provider_t* tobeDeleted = iter->next;
			pthread_mutex_unlock(mylist->mutex);
			iter->next = iter->next->next;
			return 1;
		}
		iter = iter->next;
	}
	pthread_mutex_unlock(mylist->mutex);
	return -1;

}


	

/* Get pointer to IP address of next available provider (peerip) in the iplist of the fileEntry associated with @filename, but
 not in @providerLists. Return NULL if no more peers available at this time */
provider_t* getNextAvailableProvider( fileTable_t* filetable, providerList_t* providerlist, char* fileName) {

    fileEntry_t* entry = filetable_searchFileByName(filetable, fileName);
    if (entry == NULL){
        return NULL;
    }


   	int i = 0;
    for(i = 0; i < entry->peerNum; i++){
    	//loop through all the available peer ip of that @filename
    	//check if the ip in the iplist is already in the @providerlist, if not exist then 
    	// add the first 'unseen (to providerList)' ip to provider list
        if (!PL_searchProviderByIP(providerlist, entry->iplist[i])) {
        	//cannot find, means it's new, create a provider_t and return it
        	

        	//create the porvider according to ip
        	provider_t* new = PL_createProvider(entry->iplist[i]);
            return new;
        }
    }
    return NULL;
}





int PL_destroy(providerList_t* mylist){

	provider_t* iter = mylist->head;
	pthread_mutex_lock(mylist->mutex);
	while(iter){
		provider_t* tobeDeleted = iter;
		iter = iter->next;
		free(iter);
	}
	pthread_mutex_unlock(mylist->mutex);
	free(mylist);
	return 1;
}






