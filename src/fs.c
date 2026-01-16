/*
* File system module
* 
* This module provides a file system interface for the 
* nspire-fm application. It provides functions to scan
* directories, list files, and copy files. Since Ti-Nspire
* has a Unixy file system, we can use the standard C library
* functions to interact with it.
*
* Limitations:
* - No file permissions
* - No file ownership
* - No file timestamps
* - No file links or symlinks
* - No file hard links
* - No file extended attributes
*/


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct {
    char name[256];
    int is_dir;
    unsigned int size;
} file_entry_t;

typedef struct {
    file_entry_t *entries;
    int count;
    char path[512];
} file_list_t;

// Sort helper
static int compare_entries(const void *a, const void *b) {
    file_entry_t *fa = (file_entry_t*)a;
    file_entry_t *fb = (file_entry_t*)b;
    
    // Prio 1: ".." always first
    if (strcmp(fa->name, "..") == 0) return -1;
    if (strcmp(fb->name, "..") == 0) return 1;
    
    // Prio 2: Dirs first
    if (fa->is_dir && !fb->is_dir) return -1;
    if (!fa->is_dir && fb->is_dir) return 1;
    
    // Prio 3: Alphabetical
    return strcasecmp(fa->name, fb->name);
}

/* Free file list. Entries are not freed. */

void fs_free(file_list_t *list) {
    if (list->entries) free(list->entries);
    list->entries = NULL;
    list->count = 0;
}

int fs_scan(const char *path, file_list_t *list) {
    fs_free(list);
    strncpy(list->path, path, sizeof(list->path));
    
    DIR *d = opendir(path);
    if (!d) return -1;
    
    struct dirent *dir;
    int capacity = 16;
    list->entries = malloc(sizeof(file_entry_t) * capacity);
    list->count = 0;
    
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0) continue;
        
        if (list->count >= capacity) {
            capacity *= 2;
            list->entries = realloc(list->entries, sizeof(file_entry_t) * capacity);
        }
        
        file_entry_t *entry = &list->entries[list->count];
        strncpy(entry->name, dir->d_name, sizeof(entry->name) - 1);
        entry->name[255] = '\0';
        
        if (strcmp(dir->d_name, "..") == 0) {
            entry->is_dir = 1;
            entry->size = 0;
        } else {
            // Stat to check type
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dir->d_name);
            
            struct stat st;
            if (stat(fullpath, &st) == 0) {
                entry->is_dir = S_ISDIR(st.st_mode);
                entry->size = st.st_size;
            } else {
                entry->is_dir = 0;
                entry->size = 0;
            }
        }
        
        list->count++;
    }
    
    closedir(d);
    
    qsort(list->entries, list->count, sizeof(file_entry_t), compare_entries);
    return 0;
}

/*
* Copies a file from one location to another.
* It takes a source path and a destination path as arguments.
* It uses the standard C library functions to copy the file.
*/

int fs_copy_file(const char *src_path, const char *dst_path) {
    FILE *in = fopen(src_path, "rb");
    if (!in) return -1;
    
    FILE *out = fopen(dst_path, "wb");
    if (!out) {
        fclose(in);
        return -2;
    }
    
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return -3; // Write error
        }
    }
    
    fclose(in);
    fclose(out);
    return 0;
}
