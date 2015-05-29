#include <unistd.h>
#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/filetable.h"


void fill_filetable(fileTable_t* filetable, char *dir, char* prefix)
{
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
            fileEntry_t* file_entry= filetable_createFileEntry(path, 0, (unsigned long int) statbuf.st_ctime, DIRECTORY);
            filetable_appendFileEntry(filetable, file_entry);
            free(path);

            // Get the new prefix for the recursive call
            int new_prefix_len = strlen(prefix) + strlen("/") + strlen(dir) + 1; 
            new_prefix = (char *) malloc(new_prefix_len);
            strcpy(new_prefix, prefix);
            strcat(new_prefix, "/");
            strcat(new_prefix, entry -> d_name);

            // Recurse through the subdirectory
            fill_filetable(filetable, entry->d_name, new_prefix);
        }
        else {
          //get the file_path as a string
          int path_len = strlen(prefix) + strlen("/") + strlen(entry -> d_name) + strlen("/") + 1;  // prefix + directory_name/termination_char            
          char* path = malloc(path_len);
          memset(path, 0, path_len);
          sprintf(path, "%s/%s/", prefix, entry -> d_name);
          
          //add the entry to the file table
          fileEntry_t* file_entry= filetable_createFileEntry(path, statbuf.st_size, (unsigned long int) statbuf.st_ctime, FILE);
          filetable_appendFileEntry(filetable, file_entry);
          free(path);
        }
    }
    chdir("..");
    if (new_prefix) free(new_prefix);
    closedir(dp);
}

fileTable_t* create_local_filetable(char* root_dir) {
  fileTable_t* filetable = filetable_init();
  fill_filetable(filetable, root_dir, ".");
  return filetable;
}

// /*  Now we move onto the main function.  */
// int main(int argc, char* argv[]) {
//     char *topdir, pwd[10]=".";
//     topdir=pwd;

//     fileTable_t* filetable = create_local_filetable(topdir);
//     filetable_printFileTable(filetable);

//     return 0;
// }
