#ifndef FS_H
#define FS_H

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

int fs_scan(const char *path, file_list_t *list);
void fs_free(file_list_t *list);
int fs_copy_file(const char *src_path, const char *dst_path);
int fs_generate_copy_name(const char *original_path, char *out_path, size_t out_size);

#endif
