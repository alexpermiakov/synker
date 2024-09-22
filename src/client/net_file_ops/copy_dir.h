#ifndef COPY_DIR_H
#define COPY_DIR_H

#include "data_structures/hash_table.h"

void copy_dir (int client_fd, char *src_file_path, char *dst_file_path, HashTable *wd_to_path, HashTable *path_to_wd, int inotify_fd);

#endif