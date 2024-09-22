#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "utils/file_utils.h"
#include "copy_dir.h"
#include "copy_file.h"
#include "delete_dir.h"
#include "delete_file.h"
#include "client/helpers/inotify.h"
#include "data_structures/hash_table.h"

void copy_dir (int client_fd, char *src_file_path, char *dst_file_path, HashTable *wd_to_path, HashTable *path_to_wd, int inotify_fd) {
  DIR *dir = opendir(src_file_path);

  if (dir == NULL) {
    perror("opendir");
    exit(1);
  }

  inotify_add_watch_recursively(wd_to_path, path_to_wd, inotify_fd, src_file_path);

  ssize_t attr_size = sizeof(file_attrs_t);
  char file_attr_buffer[attr_size];
  file_attrs_t file_attrs;
  
  extract_file_metadata(src_file_path, &file_attrs);
  strcpy(file_attrs.file_path, dst_file_path);
  file_attrs.operation = CREATE_DIR;
  serialize_file_attrs(&file_attrs, file_attr_buffer);

  printf("Send directory attributes %s\n", dst_file_path);
  
  if (write(client_fd, file_attr_buffer, attr_size) < 0) {
    fprintf(stderr, "Failed to send file attributes\n");
    close(client_fd);
    exit(1);
  }

  struct dirent *entry;

  while((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char src_path[PATH_MAX];
    char sub_dst_file_path[PATH_MAX];

    snprintf(src_path, sizeof(src_path), "%s/%s", src_file_path, entry->d_name);
    snprintf(sub_dst_file_path, sizeof(sub_dst_file_path), "%s/%s", dst_file_path, entry->d_name);

    if (entry->d_type == DT_DIR) {
      copy_dir(client_fd, src_path, sub_dst_file_path, wd_to_path, path_to_wd, inotify_fd);
    } else {
      copy_file(client_fd, src_path, sub_dst_file_path);
    }
  }

  closedir(dir);
}