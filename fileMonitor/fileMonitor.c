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
char* directory = NULL;

/*
*the main file monitor thread, watches for changes
*
*@arg: the function pointers required by the localFileAlerts object type
*
*/
void *fileMonitorThread(void* arg) {
	//cast args to be the function pointers given by the client
	localFileAlerts* funcs = (localFileAlerts*)arg;

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

	FileInfo_table_print(ftable);

	int i;
	for(i = 0; i < ftable->num_files; i++) {
		funcs->fileAdded(ftable->table[i].filepath);
	}
	//wait a set interval time before checking the directory again
	sleep(MONITOR_POLL_INTERVAL);

	while(1) {
		//get the updated table
		FileInfo_table* newtable = getAllFilesInfo();
		//get a comparison and call the necessary functions
		FilesInfo_UpdateAlerts(newtable, funcs);
		//set ftable to the updated version
		free(ftable);
		ftable = newtable;
		//print table for testing
		FileInfo_table_print(ftable);
		//wait a set interval time before checking the directory again
		sleep(MONITOR_POLL_INTERVAL);
	}


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
		exit(EXIT_FAILURE);
	}

	FileInfo myInfo;
	myInfo.filepath = filename;
	myInfo.size = statinfo.st_size;
	myInfo.lastModifyTime = statinfo.st_mtime;

	free(filepath);

	return myInfo;
}
/*
*Gets a table of file info for the directory
*
*Returns a FileInfo_table pointer
*/
FileInfo_table* getAllFilesInfo() {
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
				num_files++;
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
				allfiles->table[idx] = getFileInfo(ent->d_name);
				idx++;
			}
			if(idx == num_files) {
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

	int i;
	int idx;
	for(i = 0; i < ftable->num_files; i++) {
		filepath = ftable->table[i].filepath;
		if(FilesInfo_table_search(filepath, newtable) == -1) {
			funcs->fileDeleted(filepath);
		}
	}

	for(i = 0; i < newtable->num_files; i++) {
		filepath = newtable->table[i].filepath;
		idx = FilesInfo_table_search(filepath, ftable);
		if(idx == -1) {
			funcs->fileAdded(filepath);
		}
		else if (ftable->table[idx].lastModifyTime != newtable->table[i].lastModifyTime) {
			funcs->fileModified(filepath);
		}
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
	directory = calloc(1, strlen(buf) * sizeof(char));
	strcpy(directory, buf);
	fclose(config);
}
/*
*frees global variables
*/
void FileMonitor_freeAll() {
	/*int i;
	for (i = 0; i < ftable->num_files; i++) {
		free(ftable->table[i]);
	}*/
	free(ftable);
	free(directory);
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

=======
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

//global variable holding all the file info currently recorded
FileInfo_table* ftable;
char* directory = NULL;

/*
*the main file monitor thread, watches for changes
*
*@arg: the function pointers required by the localFileAlerts object type
*
*/
void *fileMonitorThread(void* arg) {
	//cast args to be the function pointers given by the client
	localFileAlerts* funcs = (localFileAlerts*)arg;

	//read the config file for necessary information
	readConfigFile("config");

	//check that directory was set
	if(!directory) {
		printf("Directory not listed in config\n");
		return NULL;
	}

	//fill in the first table
	ftable = getAllFilesInfo();

	int i;
	for(i = 0; i < ftable->num_files; i++) {
		funcs->fileAdded(ftable[i].filepath);
	}
	//wait a set interval time before checking the directory again
	sleep(MONITOR_POLL_INTERVAL);

	while(1) {
		//get the updated table
		FileInfo* newtable = getAllFilesInfo();
		//get a comparison and call the necessary functions
		FilesInfo_UpdateAlerts(newtable, funcs);
		//set ftable to the updated version
		free(ftable);
		ftable = newtable;
		//wait a set interval time before checking the directory again
		sleep(MONITOR_POLL_INTERVAL);
	}


}
/*
*Gets the file info for a given filename
*
*@filename:the filename to return info for
*
*Returns a FileInfo struct
*/

FileInfo getFileInfo(char* filename) {
	struct stat statinfo;
	if(stat(filename, &statinfo) == -1) {
		perror("Stat error");
		exit(EXIT_FAILURE);
	}

	FileInfo myInfo;
	myInfo.filepath = filename;
	myInfo.size = statinfo.st_size;
	myInfo.lastModifyTime = statinfo.st_mtime;

	return myInfo;
}
/*
*Gets a table of file info for the directory
*
*Returns a FileInfo_table pointer
*/
FileInfo_table* getAllFilesInfo() {
	int num_files = 0;
	//http://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
	DIR *dir;
	struct dirent *ent;
	struct stat *entinfo;
	if ((dir = opendir(directory)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			stat(ent->d_name, entinfo);
			if(entinfo->st_mode == S_IFREG) {
				num_files++;
			}
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
			stat(ent->d_name, entinfo);
			if(entinfo->st_mode == S_IFREG) {
				*allfiles->table[idx] = getFileInfo(ent->d_name);
				idx++;
			}
			if(idx == num_files) {
				break;
			}
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
*Sends the necessary alerts by comparing the old and new table
*
*@newtable:the newtable after the polling interval
*@funcs: the functions to call based on the update's results
*/
void FilesInfo_UpdateAlerts(FileInfo_table* newtable, localFileAlerts* funcs) {
	char* filepath;

	int i;
	int idx;
	for(i = 0; i < ftable->num_files; i++) {
		filepath = ftable->table[i]->filepath;
		if(FilesInfo_table_search(filepath, newtable) == -1) {
			funcs->fileDeleted(filepath);
		}
	}

	for(i = 0; i < newtable->num_files; i++) {
		filepath = newtable->table[i]->filepath;
		idx = FilesInfo_table_search(filepath, ftable);
		if(idx == -1) {
			funcs->fileAdded(filepath);
		}
		else if (ftable->table[idx]->lastModifyTime != newtable->table[i]->lastModifyTime) {
			funcs->fileModified(filepath);
		}
	}

}
/*
*Reads the config file and stores the directory path
*
*@filename: name of the config file
*
*/
void readConfigFile(char* filename) {
	//set directory to the directory specified in the config file
	//directory = 
}
/*
*frees global variables
*/
void FileMonitor_freeAll() {
	int i;
	for (i = 0; i < ftable->num_files) {
		free(ftable->table[i]);
	}
	free(ftable);
	free(directory);
}