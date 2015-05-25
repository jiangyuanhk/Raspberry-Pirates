typedef struct {
  char* filepath;			//path of the file
  int size;					//size of the file
  unsigned long int lastModifyTime; //time stamp
} FileInfo;

typedef struct {
	int num_files;			//number of files in the table
  	FileInfo* table;		//the table of files
} FileInfo_table;

typedef struct {
	void (*fileAdded)(char *filename);
	void (*fileModified)(char *filename);
	void (*fileDeleted)(char *filename);
} localFileAlerts;

/*
*the main file monitor thread, watches for changes
*
*@arg: the function pointers required by the localFileAlerts object type
*
*/
void *fileMonitorThread(void* arg);
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
*Sends the necessary alerts by comparing the old and new table
*
*@newtable:the newtable after the polling interval
*@funcs: the functions to call based on the update's results
*/
void FilesInfo_UpdateAlerts(FileInfo_table* newtable, localFileAlerts* funcs);
/*
*Reads the config file and stores the directory path
*
*@filename: name of the config file
*
*/
void readConfigFile(char* filename);
/*
*frees global variables
*/
void FileMonitor_freeAll();