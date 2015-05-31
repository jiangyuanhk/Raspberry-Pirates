#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "../common/constants.h"
#include "fileMonitor.h"



//global variable holding all the file info currently recorded
FileInfo_table* ftable;
FileBlockList* blockList;
char* directory = NULL;
int running = 1;

/*
*the main file monitor thread, watches for changes
*
*@arg: the function pointers required by the localFileAlerts object type
*
*/
void *fileMonitorThread(void* arg) {
	//cast args to be the function pointers given by the client
	localFileAlerts* funcs = (localFileAlerts*)arg;

	blockList = NULL;
	//read the config file for necessary information
	readConfigFile("./config");

	//check that directory was set
	if(!directory) {
		printf("Directory not listed in config\n");
		return NULL;
	}

	printf("Watching directory %s\n", directory);

	//fill in the first table
	ftable = getAllFilesInfo();

	int i;
	for(i = 0; i < ftable->num_files; i++) {
		//funcs->fileSync(ftable->table[i].filepath);
		funcs->fileAdded(ftable->table[i].filepath);
	}
	//sync using the register packet
	funcs->fileSync();
	//funcs->fileAdded(ftable->table[i].filepath);
	funcs->filesChanged();
	//wait a set interval time before checking the directory again
	sleep(MONITOR_POLL_INTERVAL);

	while(running) {
		//print table for testing
		//FileInfo_table_print(ftable);
		//get the updated table
		FileInfo_table* newtable = getAllFilesInfo();
		//get a comparison and call the necessary functions
		FilesInfo_UpdateAlerts(newtable, funcs);
		//set ftable to the updated version
		FileInfo_table_destroy(ftable);
		ftable = newtable;
		//wait a set interval time before checking the directory again
		sleep(MONITOR_POLL_INTERVAL);
	}

	FileMonitor_freeAll();

	return NULL;
}
/*
* Frees space allocated for a File info table
*
*@table: the table to free
*/
void FileInfo_table_destroy(FileInfo_table* table) {
	int i;
	for (i = 0; i < table->num_files; i++) {
		free(table->table[i].filepath);
	}
	free(table->table);
	free(table);
}
/*
*Signals the file monitor to close
*/
void FileMonitor_close() {
	running = 0;
}
/*
*Gets the file info for a given filename
*
*@filename:the filename to return info for
*
*Returns a FileInfo struct
*/

FileInfo getFileInfo(char* filename) {
	char* filepath = calloc(1, (strlen(directory) + strlen(filename) + 1) * sizeof(char));
	sprintf(filepath, "%s%s", directory, filename);
	struct stat statinfo;
	if(stat(filepath, &statinfo) == -1) {
		free(filepath);
		perror("Stat error");
		printf("%s\n", filepath)
		exit(EXIT_FAILURE);
	}

	FileInfo myInfo;
	myInfo.filepath = calloc(1, strlen(filename)*sizeof(char)+1);
	strcpy(myInfo.filepath, filename);
	myInfo.size = statinfo.st_size;
	myInfo.type = statinfo.st_mode;
	myInfo.lastModifyTime = statinfo.st_mtime;

	free(filepath);

	return myInfo;
}
/*
* Recursively counts all files in the subdirectory
*
*@subdirectory_path: the path of the current subdirectory
*
*returns the number of files in this subdirectory and its subdirectories
*/
int FileInfo_table_SubdirectoryFileCount(char* subdirectory_path) {
	char* extension;
	int num_files = 0;
	//http://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
	DIR *dir;
	struct dirent *ent;
	struct stat entinfo;
	if ((dir = opendir(subdirectory_path)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			char* filepath = calloc(1, (strlen(subdirectory_path) + strlen(ent->d_name) + 2) * sizeof(char));
			sprintf(filepath, "%s/%s", subdirectory_path, ent->d_name);
			stat(filepath, &entinfo);
			if(S_ISREG(entinfo.st_mode)) {
				extension = strrchr(ent->d_name, '.');
				if(extension) {
					if(strcmp(extension, ".swp") == 0) {
						free(filepath);
						continue;
					}
				}
				num_files++;
			}
			else if (S_ISDIR(entinfo.st_mode)) {
				if(strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
					num_files+=FileInfo_table_SubdirectoryFileCount(filepath) + 1;
				}
			}
			free(filepath);
		}
		closedir(dir);
	}
	else {
		printf("Failed to open directory\n");
		return 0;
	}
	return num_files;
}
/*
* Recursively adds all files in the subdirectory
*
*@allfiles: the file info table to add to
*@subdirectory_path: the path of the current subdirectory
*/
int FileInfo_table_Subdirectory(FileInfo_table* allfiles, char* subdirectory_path, int idx) {
	char* extension;
	DIR *dir;
	struct dirent *ent;
	struct stat entinfo;
	char* subdir = calloc(1, (strlen(subdirectory_path) + strlen(directory) + 2) * sizeof(char));
	sprintf(subdir, "%s%s/", directory, subdirectory_path);
	if ((dir = opendir(subdir)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			char* filepath = calloc(1, (strlen(subdir) + strlen(ent->d_name) + 2) * sizeof(char));
			sprintf(filepath, "%s/%s", subdir, ent->d_name);
			char* relpath = calloc(1, (strlen(subdirectory_path) + strlen(ent->d_name) + 2) * sizeof(char));
			sprintf(relpath, "%s/%s", subdirectory_path, ent->d_name);
			stat(filepath, &entinfo);
			if(S_ISREG(entinfo.st_mode)) {
				extension = strrchr(ent->d_name, '.');
				if(extension) {
					if(strcmp(extension, ".swp") == 0) {
						free(filepath);
						continue;
					}
				}
				allfiles->table[idx] = getFileInfo(relpath);
				idx++;
			}
			else if(S_ISDIR(entinfo.st_mode)) {
				if(strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
					allfiles->table[idx] = getFileInfo(relpath);
					idx++;
					idx = FileInfo_table_Subdirectory(allfiles, relpath, idx);
					
				}
			}
			if(idx == allfiles->num_files) {
				break;
			}
			free(relpath);
			free(filepath);
		}
		closedir(dir);
	}
	else {
		printf("Failed to open directory\n");
		return 0;
	}
	free(subdir);
	return idx;
}
/*
*Gets a table of file info for the directory
*
*Returns a FileInfo_table pointer
*/
FileInfo_table* getAllFilesInfo() {
	char* extension;
	int num_files = 0;
	//http://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
	DIR *dir;
	struct dirent *ent;
	struct stat entinfo;
	if ((dir = opendir(directory)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			char* filepath = calloc(1, (strlen(directory) + strlen(ent->d_name) + 1) * sizeof(char));
			sprintf(filepath, "%s%s", directory, ent->d_name);
			stat(filepath, &entinfo);
			if(S_ISREG(entinfo.st_mode)) {
				extension = strrchr(ent->d_name, '.');
				if(extension) {
					if(strcmp(extension, ".swp") == 0) {
						free(filepath);
						continue;
					}
				}
				num_files++;
			}
			else if (S_ISDIR(entinfo.st_mode)) {
				if(strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
					num_files+=FileInfo_table_SubdirectoryFileCount(filepath) + 1;
				}
			}
			free(filepath);
		}
		closedir(dir);
	}
	else {
		printf("Failed to open directory\n");
		return NULL;
	}

	FileInfo_table* allfiles = calloc(1, sizeof(FileInfo_table));
	allfiles->num_files = num_files;
	allfiles->table = calloc(num_files, sizeof(FileInfo));

	int idx = 0;
	if ((dir = opendir(directory)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			char* filepath = calloc(1, (strlen(directory) + strlen(ent->d_name) + 1) * sizeof(char));
			sprintf(filepath, "%s%s", directory, ent->d_name);
			stat(filepath, &entinfo);
			if(S_ISREG(entinfo.st_mode)) {
				extension = strrchr(ent->d_name, '.');
				if(extension) {
					if(strcmp(extension, ".swp") == 0) {
						free(filepath);
						continue;
					}
				}
				allfiles->table[idx] = getFileInfo(ent->d_name);
				idx++;
			}
			else if(S_ISDIR(entinfo.st_mode)) {
				if(strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
					allfiles->table[idx] = getFileInfo(ent->d_name);
					idx++;
					idx = FileInfo_table_Subdirectory(allfiles, ent->d_name, idx);
				}
			}
			if(idx == num_files) {
				free(filepath);
				break;
			}
			free(filepath);
		}
		closedir(dir);
	}
	else {
		printf("Failed to open directory\n");
		return NULL;
	}

	return allfiles;
}
/*
*Searches a fileinfo_table struct for a file with the given name
*
*@name: the name of the file to search for
*@table: the table to search in
*
*returns an index into that table or -1 if not found
*/
int FilesInfo_table_search(char* name, FileInfo_table* fItable) {
	int i;
	for(i = 0; i < fItable->num_files; i++) {
		if(strcmp(fItable->table[i].filepath, name) == 0) {
			return i;
		}
	}
	return -1;
}
/*
*Sends the necessary alerts by comparing the old and new table
*
*@newtable:the newtable after the polling interval
*@funcs: the functions to call based on the update's results
*/
void FilesInfo_UpdateAlerts(FileInfo_table* newtable, localFileAlerts* funcs) {
	char* filepath;
	char* filename;

	int i;
	int idx;
	int change = 0;
	for(i = 0; i < ftable->num_files; i++) {
		filename = ftable->table[i].filepath;
		filepath = calloc(1, (strlen(directory) + strlen(filename) + 1) * sizeof(char));
		sprintf(filepath, "%s%s", directory, filename);

		if(FilesInfo_table_search(filename, newtable) == -1 && !FileBlockList_Search(filepath, EVENT_DELETED)) {
			printf("File deleted: %s\n",filename);
			change = 1;
			funcs->fileDeleted(filepath);
		}
		else {
			free(filepath);
		}
	}

	for(i = 0; i < newtable->num_files; i++) {
		
		filename = newtable->table[i].filepath;
		char* filepath = calloc(1, (strlen(directory) + strlen(filename) + 1) * sizeof(char));
		sprintf(filepath, "%s%s", directory, filename);

		idx = FilesInfo_table_search(filename, ftable);
		if(idx == -1) {
			if(!FileBlockList_Search(filepath, EVENT_ADDED)) {
				printf("File added: %s\n",filename);
				change = 1;
				funcs->fileAdded(filename);
			}
		}
		else if (ftable->table[idx].lastModifyTime != newtable->table[i].lastModifyTime  && !FileBlockList_Search(filepath, EVENT_MODIFIED)) {
			printf("File updated: %s\n",filename);
			change = 1;
			printf("Filename before file modified: %s\n", filename);
			printf("Filepath: %s\n", filepath);
			funcs->fileModified(filename);
		}
		free(filepath);
	}
	if(change) {
		funcs->filesChanged();
	}

}
/*
*Reads the config file and stores the directory path
*
*@filename: name of the config file
*
*/
char* readConfigFile(char* filename) {
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
	directory = calloc(1, strlen(buf) * sizeof(char) + 1);
	strcpy(directory, buf);
	fclose(config);

	return directory;
}
/*
* Frees the global variables
*/
void FileMonitor_freeAll() {
	//Free the file info table
	FileInfo_table_destroy(ftable);
	//free the directory string
	free(directory);
	//Free the block list
	FileBlockList* toFree = blockList;
	if (toFree) {
		FileBlockList* next = toFree->next;
		while(next) {
			free(toFree->filepath);
			free(toFree);
			toFree = next;
			next = next->next;
		} 
		free(toFree->filepath);
		free(toFree);
	}

	ftable = NULL;
	directory = NULL;
	blockList = NULL;
}
/*
* Adds a file from the block list
*
*@toAppend: the item to be added
*
*/
void FileBlockList_Append(FileBlockList* toAppend) {
	FileBlockList* curr = blockList;
	if(!curr) {
		blockList = toAppend;
	}
	else {
		while(curr->next) {
			curr = blockList->next;
		}
		curr->next = toAppend;
	}
}
/*
* Finds a file and event int the block list
*
*@filepath: name and path of the file
*@event: the specific event to remove
*
*returns 0 on not found, and 1 on found
*/
int FileBlockList_Search(char* filepath, int event) {
	FileBlockList* curr = blockList;
	if(!curr) {
		return 0;
	}
	while(curr->next) {
		if(strcmp(filepath, curr->filepath) == 0) {
			if(curr->event == event) {
				return 1;
			}
		}
		curr = curr->next;
	}
	if(strcmp(filepath, curr->filepath) == 0) {
		if(curr->event == event) {
			return 1;
		}
	}
	return 0;
}
/*
* Removes a file from the block list
*
*@filepath: name and path of the file
*@event: the specific event to remove
*
*returns 0 on failure, 1 on success
*/
int FileBlockList_Remove(char* filepath, int event) {
	FileBlockList* prev = NULL;
	FileBlockList* curr = blockList;
	int ret = 0;
	if(!curr) {
		return ret;
	}
	while(curr) {
		//will do the wrong thing if file name is part of a larger file name
		if(strncmp(filepath, curr->filepath, strlen(filepath)) == 0) {
			if(curr->event == event) {
				ret = 1;
				if(prev) {
					prev->next = curr->next;
					free(curr->filepath);
					free(curr);
					curr = prev->next;
				}
				else {
					blockList = curr->next;
					free(curr->filepath);
					free(curr);
					curr = blockList;
				}
				continue;
			}
		}
		prev = curr;
		curr = curr->next;
	}
	return ret;
}
/*
*Blocks a file from being added
*
*@filename: the name of the file excluding the directory name
*/
void blockFileAddListening(char* filename) {
	
	//char* filepath = calloc(1, (strlen(directory) + strlen(filename) + 1) * sizeof(char));
	//sprintf(filepath, "%s%s", directory, filename);

	FileBlockList* blockAdd = calloc(1, sizeof(FileBlockList));
	blockAdd->filepath = calloc(1, strlen(filename) * sizeof(char) + 1);
	strcpy(blockAdd->filepath,filename);
	blockAdd->event = EVENT_ADDED;


	FileBlockList_Append(blockAdd);

	blockFileWriteListening(filename);

}
/*
*Blocks a file from being updated
*
*@filename: the name of the file excluding the directory name
*/
void blockFileWriteListening(char* filename) {
	
	//char* filepath = calloc(1, (strlen(directory) + strlen(filename) + 1) * sizeof(char));
	//sprintf(filepath, "%s%s", directory, filename);

	FileBlockList* blockWrite = calloc(1, sizeof(FileBlockList));
	blockWrite->filepath = calloc(1, strlen(filename) * sizeof(char) + 1);
	strcpy(blockWrite->filepath,filename);
	blockWrite->event = EVENT_MODIFIED;


	FileBlockList_Append(blockWrite);

}
/*
*Blocks a file from being deleted
*
*@filename: the name of the file excluding the directory name
*/
void blockFileDeleteListening(char* filename) {
	
	//char* filepath = calloc(1, (strlen(directory) + strlen(filename) + 1) * sizeof(char));
	//sprintf(filepath, "%s%s", directory, filename);

	FileBlockList* blockDelete = calloc(1, sizeof(FileBlockList));
	blockDelete->filepath = calloc(1, strlen(filename) * sizeof(char) + 1);
	strcpy(blockDelete->filepath,filename);
	blockDelete->event = EVENT_DELETED;

	struct stat entinfo;
	//char* thispath = calloc(1, strlen(directory) + strlen(filename) + 1);
	//sprintf(thispath, "%s%s", directory, filename);
	if(stat(filename, &entinfo)== 0) {
		if(S_ISDIR(entinfo.st_mode)) {
			DIR *dir;
			struct dirent *ent;
			if ((dir = opendir(filename)) != NULL) {
				while ((ent = readdir(dir)) != NULL) {
					if(strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
						char* filepath = calloc(1, strlen(filename) + 1 + strlen(ent->d_name));
						sprintf(filepath, "%s/%s", filename, ent->d_name);
						blockFileDeleteListening(filepath);
						free(filepath);
					}
				}
			}
		}
	}
	else {
		printf("Stat failed on path %s\n", filename);
	}
	//free(thispath);


	FileBlockList_Append(blockDelete);

}
/*
*unblocks a file from being added
*
*@filename: the name of the file excluding the directory name
*/
int unblockFileAddListening(char* filename) {
	
	//char* filepath = calloc(1, (strlen(directory) + strlen(filename) + 1) * sizeof(char));
	//sprintf(filepath, "%s%s", directory, filename);

	int event = EVENT_ADDED;

	int ret = unblockFileWriteListening(filename);
	if(!ret) {
		printf("Unexpected unblock failure for file %s", filename);
	}
	ret += FileBlockList_Remove(filename, event);

	return ret;

}
/*
*unblocks a file from being updated
*
*@filename: the name of the file excluding the directory name
*/
int unblockFileWriteListening(char* filename) {
	
	//char* filepath = calloc(1, (strlen(directory) + strlen(filename) + 1) * sizeof(char));
	//sprintf(filepath, "%s%s", directory, filename);

	int event = EVENT_MODIFIED;

	return FileBlockList_Remove(filename, event);

}
/*
*unblocks a file from being deleted
*
*@filename: the name of the file excluding the directory name
*/
int unblockFileDeleteListening(char* filename) {
	
	//char* filepath = calloc(1, (strlen(directory) + strlen(filename) + 1) * sizeof(char));
	//sprintf(filepath, "%s%s", directory, filename);

	int event = EVENT_DELETED;

	return FileBlockList_Remove(filename, event);

}
/*
*Prints a FileInfo_table for testing
*
*@toPrint: the table to print
*/
void FileInfo_table_print(FileInfo_table* toPrint) {
	printf("This file table has %d files. These files are as follows:\n\n", toPrint->num_files);
	int i;
	for(i = 0; i < toPrint->num_files; i++) {
		printf("\tFile Name: %s\t|\tFile Size: %d\n", toPrint->table[i].filepath, toPrint->table[i].size);
	}
	printf("\n\n");
}
