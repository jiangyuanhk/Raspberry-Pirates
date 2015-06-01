/*
File: file_monitor.c

Description: Defines functions to help the peer monitor files.
*/

#include "file_monitor.h"
#include "../common/filetable.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>


void fill_filetable(fileTable_t* filetable, char *dir, char* prefix, int depth) {
  DIR *dp;
  struct dirent *entry;
  struct stat statbuf;

  if((dp = opendir(dir)) == NULL) {
      fprintf(stderr,"cannot open directory: %s\n", dir);
      return;
  }
  char* new_prefix = NULL;
  chdir(dir);
  while((entry = readdir(dp)) != NULL) {
      lstat(entry->d_name,&statbuf);
      if(S_ISDIR(statbuf.st_mode)) {
          /* Found a directory, but ignore . and .. */
          if(strcmp(".",entry->d_name) == 0 || 
              strcmp("..",entry->d_name) == 0)
              continue;

          //get the file_path as a string
          int path_len = strlen(prefix) + strlen("/") + strlen(entry -> d_name) + strlen("/") + 1;  // prefix + directory_name/termination_char            
          char* path = malloc(path_len);
          memset(path, 0, path_len);
          sprintf(path, "%s/%s/", prefix, entry -> d_name);
          
          //add the entry to the file table
          fileEntry_t* file_entry= filetable_createFileEntry(path, 0, (unsigned int) statbuf.st_mtime, DIRECTORY);
          filetable_appendFileEntry(filetable, file_entry);
          free(path);

          // Get the new prefix for the recursive call
          int new_prefix_len = strlen(prefix) + strlen("/") + strlen(dir) + 1; 
          new_prefix = (char *) malloc(new_prefix_len);
          strcpy(new_prefix, prefix);
          strcat(new_prefix, "/");
          strcat(new_prefix, entry -> d_name);

          // Recurse through the subdirectory
          fill_filetable(filetable, entry->d_name, new_prefix, depth + 1);
      }
      else {
        //get the file_path as a string
        int path_len = strlen(prefix) + strlen("/") + strlen(entry -> d_name) + strlen("/") + 1;  // prefix + directory_name/termination_char            
        char* path = malloc(path_len);
        memset(path, 0, path_len);
        sprintf(path, "%s/%s", prefix, entry -> d_name);
        
        //add the entry to the file table
        fileEntry_t* file_entry= filetable_createFileEntry(path, statbuf.st_size, (unsigned int) statbuf.st_mtime, REGULAR_FILE);
        filetable_appendFileEntry(filetable, file_entry);
        free(path);
      }
  }
  if (depth != 0) chdir("..");		//avoid issue of changing the working directory
  if (new_prefix) free(new_prefix);
  closedir(dp);
}

fileTable_t* create_local_filetable(char* root_dir) {
  fileTable_t* filetable = filetable_init();
  fill_filetable(filetable, root_dir, ".", 0);
  chdir("-");
  return filetable;
}


/*
*Reads the config file and stores the directory path
*
*@filename: name of the config file
*
*/
char* read_config_file(char* filename) {
	//set directory to the directory specified in the config file
	FILE* config;
	//TODO: not sure how globals are defined .. 
	char buf[80];
	if ((config = fopen(filename,"r")) == NULL) {
		printf("Config file not found\n");
		return NULL;
	}
	if (fgets(buf, 79, config) == NULL) {
		printf("No line read from config file\n");
		return NULL;
	}
	char* directory = calloc(1, strlen(buf) * sizeof(char));
	strcpy(directory, buf);
	fclose(config);
	return directory;
}

blockList_t* blocklist_init() {
	blockList_t* blocklist = malloc(sizeof(blockList_t));
	blocklist -> head = NULL;
	blocklist -> tail = NULL;
	blocklist -> size = 0;

	pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
	blocklist -> mutex = mutex;

	return blocklist;
}

/*
  Function to initialize a file entry.
*/
file_t* create_blocklist_entry(char* filepath) {
  file_t* file = malloc(sizeof(file_t));
  memset(file -> file_name, 0, FILE_NAME_MAX_LEN);
  memcpy(file -> file_name, filepath, strlen(filepath) + 1);
  return file;
}


void add_file_to_blocklist(blockList_t* blocklist, char* filepath) {

  file_t* file = malloc(sizeof(file_t));
  memset(file -> file_name, 0, FILE_NAME_MAX_LEN);
  memcpy(file -> file_name, filepath, strlen(filepath) + 1);

	pthread_mutex_lock(blocklist -> mutex);

  //if the table is empty, set the new file entry to be the head and tail
  if (blocklist -> size == 0) {
		blocklist -> head = file;
    blocklist -> tail = file;
	} 

  //otherwise, append the new file entry to the end of the list and make the new file the tail
  else {
		blocklist -> tail -> next = file;
		blocklist -> tail = file;
	}

	blocklist -> size ++;

	pthread_mutex_unlock(blocklist -> mutex);
	return;
}

/*
	Function to search the file block list for an entry.  If it finds the entry,
	it removes the entry and returns 1.  Otherwise, it returns -1.
*/
int find_in_blocklist(blockList_t* blocklist, char* filepath) {
	file_t* iter = blocklist -> head;

  while(iter){
    if(strcmp(iter->file_name, filepath) == 0){
      return 1;
    }
    iter = iter -> next;
  }
  return -1;
 }


//Assumes that find in block list returned 1.
int remove_from_blocklist(blockList_t* blocklist, char* filepath) {

	if(blocklist -> size == 0) return -1; //table is zero-size

	pthread_mutex_lock(blocklist -> mutex);
	file_t* file = blocklist -> head;
	
	//check if table head needs to be replaced
	if(strcmp(file -> file_name, filepath) == 0) {
		
		//if the head is the only file, updated the head and tail pointers to be NULL
		if (blocklist -> size == 1) {
			blocklist -> head = NULL;
			blocklist -> tail = NULL;
		}

		//otherwise update the head to be the file after the head
		else {
			blocklist -> head = blocklist -> head -> next;
		}

    blocklist -> size -= 1;
		
    free(file);
    pthread_mutex_unlock(blocklist -> mutex);

		return 1;
	}

  //otherwise, the file is either in the list and not the head or is not in the list
  else {
    file_t* prev_file = file;
    file = file -> next;

    //looping until reach the end of the list or find the file
    while(file != NULL) {

      //if find the file in the list, remove it and update pointers
      if (strcmp(file -> file_name, filepath) == 0) {
        prev_file -> next = file -> next;

        //if the file is the tail, update the tail to be the previous file in the list
        if (file == blocklist -> tail) {
          blocklist -> tail = prev_file;
        }

        blocklist -> size -= 1;

        free(file);
        pthread_mutex_unlock(blocklist -> mutex);

        return 1;
      }

      //move the pointers to the next pair of files.
      prev_file = file;
      file = file -> next;
    }

    //if reach here, then the file is not found and return -1
    pthread_mutex_unlock(blocklist -> mutex);
    return -1;
	}

}







