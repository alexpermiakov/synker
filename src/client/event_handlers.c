#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>

#include "data_structures/hash_table.h"
#include "utils/string_utils.h"
#include "client/helpers/epoll.h"
#include "client/helpers/inotify.h"
#include "client/net_file_ops/delete_file.h"
#include "client/net_file_ops/delete_dir.h"
#include "client/net_file_ops/copy_file.h"
#include "client/net_file_ops/copy_dir.h"

void create_handler(int client_fd, struct inotify_event *event, char *watched_dir, char *dst_to_dir_path, HashTable *wd_to_path, HashTable *path_to_wd, int inotify_fd) {       
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);
  char *src_file_path = get_src_path(base_path, event->name);
  char *dst_file_path = get_dst_path(src_file_path, watched_dir, dst_to_dir_path);

  if (event->mask & IN_ISDIR) {
    copy_dir(client_fd, src_file_path, dst_file_path, wd_to_path, path_to_wd, inotify_fd);
  } else {
    copy_file(client_fd, src_file_path, dst_file_path);
  }

  free(dst_file_path);
}

void modify_handler(int client_fd, struct inotify_event *event, char *watched_dir, char *dst_to_dir_path, HashTable *wd_to_path, HashTable *path_to_wd, int inotify_fd) {
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);
  char *src_file_path = get_src_path(base_path, event->name);
  char *dst_file_path = get_dst_path(src_file_path, watched_dir, dst_to_dir_path);

  printf("File %s was changed\n", src_file_path);

  if (event->mask & IN_ISDIR) {
    copy_dir(client_fd, src_file_path, dst_file_path, wd_to_path, path_to_wd, inotify_fd);
  } else {
    copy_file(client_fd, src_file_path, dst_file_path);
  }

  free(dst_file_path);
}

void delete_handler(int client_fd, struct inotify_event *event, char *watched_dir, char *dst_to_dir_path, HashTable *wd_to_path, HashTable *path_to_wd, int inotify_fd) {
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);

  char *src_file_path = get_src_path(base_path, event->name);
  char *dst_file_path = get_dst_path(src_file_path, watched_dir, dst_to_dir_path);

  if (event->mask & IN_ISDIR) {
    delete_dir(client_fd, dst_file_path);
    inotify_remove_watch_recursively(path_to_wd, inotify_fd, src_file_path);
  } else {
    delete_file(client_fd, dst_file_path);
  }

  free(dst_file_path);
}