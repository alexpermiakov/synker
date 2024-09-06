#include <stdio.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <stdlib.h>
#include <string.h>

#include "inotify_helper.h"
#include "hash_table.h"
#include "string_utils.h"

void inotify_add_watch_recursively(HashTable *wd_to_path, HashTable *path_to_wd, int ifd, char *new_path) {
  int wd = inotify_add_watch(ifd, new_path, IN_MODIFY | IN_DELETE | IN_CREATE);

  if (wd < 0) {
    perror("inotify_add_watch");
    exit(1);
  }

  char *key = malloc(20);
  sprintf(key, "%d", wd);
  int *new_wd = malloc(sizeof(int));
  *new_wd = wd;

  hash_table_set(wd_to_path, key, new_path);
  hash_table_set(path_to_wd, new_path, new_wd);

  DIR *dir = opendir(new_path);

  if (dir == NULL) {
    perror("opendir");
    exit(1);
  }

  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      char *key = malloc(20);
      sprintf(key, "%d", wd);
      char *base_path = hash_table_get(wd_to_path, key);
      char *sub_path = get_src_path(base_path, entry->d_name);
      inotify_add_watch_recursively(wd_to_path, path_to_wd, ifd, sub_path);
    }
  }
  
  closedir(dir);
}

void inotify_remove_watch_recursively(HashTable *path_to_wd, int ifd, char *new_path) {
  int *wd = hash_table_get(path_to_wd, new_path);
  inotify_rm_watch(ifd, *wd);
}