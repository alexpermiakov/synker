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

void create_handler(int sock_fd, struct inotify_event *event, char *watched_dir, HashTable *wd_to_path, HashTable *path_to_wd, int ifd) {       
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);
  char *src_file_path = get_src_path(base_path, event->name);
  char *dst_file_path = get_postfix(src_file_path, watched_dir);

  printf("File %s was created\n", event->name);

  if (event->mask & IN_ISDIR) {
    copy_dir(sock_fd, src_file_path, dst_file_path);
    inotify_add_watch_recursively(wd_to_path, path_to_wd, ifd, src_file_path);
  } else {
    copy_file(sock_fd, src_file_path, dst_file_path);
  }

  free(dst_file_path);
}

void modify_handler(int sock_fd, struct inotify_event *event, char *watched_dir, HashTable *wd_to_path, HashTable *path_to_wd, int ifd) {
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);
  char *src_file_path = get_src_path(base_path, event->name);
  char *dst_file_path = get_postfix(src_file_path, watched_dir);

  printf("File %s was changed\n", event->name);

  if (event->mask & IN_ISDIR) {
    copy_dir(sock_fd, src_file_path, dst_file_path);
    inotify_add_watch_recursively(wd_to_path, path_to_wd, ifd, src_file_path);
  } else {
    copy_file(sock_fd, src_file_path, dst_file_path);
  }

  free(dst_file_path);
}

void remove_handler(int sock_fd, struct inotify_event *event, char *watched_dir, HashTable *wd_to_path, HashTable *path_to_wd, int ifd) {
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);

  char *src_file_path = get_src_path(base_path, event->name);
  char *dst_file_path = get_postfix(src_file_path, watched_dir);

  if (event->mask & IN_ISDIR) {
    copy_dir(sock_fd, src_file_path, dst_file_path);
    inotify_add_watch_recursively(wd_to_path, path_to_wd, ifd, src_file_path);
  } else {
    copy_file(sock_fd, src_file_path, dst_file_path);
  }

  free(dst_file_path);
}