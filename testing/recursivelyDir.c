#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "../common/filetable.h"




//TODO: need to change the filesize filed in fielEntry_t to unsinged int

unsigned int getFilesize(const char* filename) {
    struct stat st;
    if(stat(filename, &st) != 0) {
        return 0;
    }
    return (unsigned int)st.st_size;   
}


int main(void)
{

  fileTable_t* myFileTablePtr =  filetable_init();  

  
  DIR           *d;
  struct dirent *dir;
  d = opendir(".");
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
        if(dir->d_type != DT_DIR){

            char *dot = strrchr(dir->d_name, '.');
            if(!dot || dot == dir->d_name) continue; // avoid .XXXX type, XXXX....... 
            if(strcmp(dot + 1, "temp") == 0) continue; // avoid everything .temp

            printf("name:%s, size:%u bytes\n", dir->d_name, getFilesize(dir->d_name));
            //TODO: create head of linkedlist, and append it to the filetable_append()
            
            fileEntry_t* entry = filetable_createFileEntry( getFilesize(dir->d_name), entry->filename, entry->timestamp);
            filetable_appendFileEntry(myFileTablePtr, entry);



        }
      
    }

    closedir(d);
  }

  return(0);
}






typedef struct fileEntry{
 //the size of the file
 int size;
 //the name of the file, must be unique in the same directory
 char *name;
 //the timestamp when the file is modified or created
 unsigned long int timestamp;
 //pointer to build the linked list
 struct fileEntry *pNext;

 char iplist[MAX_PEER_NUM][IP_LEN]; //tracker:  this is a list of peers' ips posessing the file
                                    //peer:     only contains ip of peer itself, put it in iplist[0]

 int peerNum;

}fileEntry_t;









