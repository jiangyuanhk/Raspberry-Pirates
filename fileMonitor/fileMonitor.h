#ifndef FILE_MONITOR_H
#define FILE_MONITOR_H

#define EVENT_ADDED 1
#define EVENT_MODIFIED 2
#define EVENT_DELETED 3


typedef struct {
  char* filepath;			//path of the file
  int size;					//size of the file
  mode_t type;				//file type
  unsigned long int lastModifyTime; //time stamp
} FileInfo;

typedef struct {
	int num_files;			//number of files in the table
  	FileInfo* table;		//the table of files
} FileInfo_table;

typedef struct fileBlockList{
	char* filepath;
	int event;

	struct fileBlockList* next;

} FileBlockList;

typedef struct {
	void (*fileAdded)(char *);
	void (*fileModified)(char *);
	void (*fileDeleted)(char *);
	//void (*fileSync)(char *);
	void (*filesChanged)(void);
} localFileAlerts;

/*
*the main file monitor thread, watches for changes
*
*@arg: the function pointers required by the localFileAlerts object type
*
*/
void *fileMonitorThread(void* arg);
/*
*Signals the file monitor to close
*/
void FileMonitor_close();
/*
*Gets the file info for a given filename
*
*@filename:the filename to return info for
*
*Returns a FileInfo struct
*/
FileInfo getFileInfo(char* filename);
/*
*Gets a table of file info for the directory
*
*Returns a FileInfo_table pointer
*/
FileInfo_table* getAllFilesInfo();
/*
*Searches a fileinfo_table struct for a file with the given name
*
*@name: the name of the file to search for
*@table: the table to search in
*
*returns an index into that table or -1 if not found
*/
int FilesInfo_table_search(char* name, FileInfo_table* fItable);
/*
*Sends the necessary alerts by comparing the old and new table
*
*@newtable:the newtable after the polling interval
*@funcs: the functions to call based on the update's results
*/
void FilesInfo_UpdateAlerts(FileInfo_table* newtable, localFileAlerts* funcs);
/*
*Reads the config file and returns the directory path as char*
*
*@filename: name of the config file
*
*/
char* readConfigFile(char* filename);
/*
*frees global variables
*/
void FileMonitor_freeAll();
/*
* Recursively counts all files in the subdirectory
*
*@subdirectory_path: the path of the current subdirectory
*
*returns the number of files in this subdirectory and its subdirectories
*/
int FileInfo_table_SubdirectoryFileCount(char* subdirectory_path);
/*
* Recursively adds all files in the subdirectory
*
*@allfiles: the file info table to add to
*@subdirectory_path: the path of the current subdirectory
*/
int FileInfo_table_Subdirectory(FileInfo_table* allfiles, char* subdirectory_path, int idx);
/*
* Adds a file from the block list
*
*@toAppend: the item to be added
*
*/
void FileBlockList_Append(FileBlockList* toAppend);
/*
* Finds a file and event int the block list
*
*@filepath: name and path of the file
*@event: the specific event to remove
*
*returns 0 on not found, and 1 on found
*/
int FileBlockList_Search(char* filepath, int event);
/*
* Removes a file from the block list
*
*@filepath: name and path of the file
*@event: the specific event to remove
*
*returns 0 on failure, 1 on success
*/
int FileBlockList_Remove(char* filepath, int event);
/*
*Blocks a file from being added
*
*@filename: the name of the file excluding the directory name
*/
void blockFileAddListening(char* filename);
/*
*Blocks a file from being updated
*
*@filename: the name of the file excluding the directory name
*/
void blockFileWriteListening(char* filename);
/*
*Blocks a file from being deleted
*
*@filename: the name of the file excluding the directory name
*/
void blockFileDeleteListening(char* filename);
/*
*unblocks a file from being added
*
*@filename: the name of the file excluding the directory name
*/
int unblockFileAddListening(char* filename);
/*
*unblocks a file from being updated
*
*@filename: the name of the file excluding the directory name
*/
int unblockFileWriteListening(char* filename);
/*
*unblocks a file from being deleted
*
*@filename: the name of the file excluding the directory name
*/
int unblockFileDeleteListening(char* filename);
/*
*Prints a FileInfo_table for testing
*
*@toPrint: the table to print
*/
void FileInfo_table_print(FileInfo_table* toPrint);

#endif
