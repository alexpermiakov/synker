#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>

#include "data_structures/hash_table.h"
#include "utils/string_utils.h"
#include "client/helpers/epoll.h"
#include "client/helpers/inotify.h"
#include "client/net_file_ops/remove_file.h"
#include "client/net_file_ops/remove_dir.h"
#include "client/net_file_ops/copy_file.h"
#include "client/net_file_ops/copy_dir.h"

void create_handler(struct inotify_event *event, char *watched_dir, char *server_url, HashTable *wd_to_path, HashTable *path_to_wd, int ifd) {       
  printf("File %s was created\n", event->name);
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);

  char *src_full_path = get_src_path(base_path, event->name);
  char *dst_full_path = get_dst_path(src_full_path, watched_dir, server_url);

  if (event->mask & IN_ISDIR) {
    copy_dir(src_full_path, dst_full_path);
    inotify_add_watch_recursively(wd_to_path, path_to_wd, ifd, src_full_path);
  } else {
    char *dst_full_path = get_dst_path(src_full_path, watched_dir, server_url);
    copy_file(src_full_path, dst_full_path);
  }

  free(dst_full_path);
}

void modify_handler(struct inotify_event *event, char *watched_dir, char *server_url, HashTable *wd_to_path) {
  printf("File %s was changed\n", event->name);
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);
  char *src_full_path = get_src_path(base_path, event->name);
  char *dst_full_path = get_dst_path(src_full_path, watched_dir, server_url);

  printf("File %s was modified\n", dst_full_path);

  copy_file(src_full_path, dst_full_path);
  free(dst_full_path);
}

void remove_handler(struct inotify_event *event, char *watched_dir, char *server_url, HashTable *wd_to_path, HashTable *path_to_wd, int ifd) {
  printf("File %s was removed\n", event->name);
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);
  char *src_full_path = get_src_path(base_path, event->name);
  char *dst_full_path = get_dst_path(src_full_path, watched_dir, server_url);  

  printf("File %s was deleted\n", dst_full_path);

  if (event->mask & IN_ISDIR) {
    inotify_remove_watch_recursively(path_to_wd, ifd, src_full_path);
    remove_dir(dst_full_path);
  } else {
    remove_file(dst_full_path);
  }

  free(dst_full_path);
}