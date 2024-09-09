#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>

#include "hash_table.h"
#include "network_operations.h"
#include "string_utils.h"
#include "event_handlers.h"
#include "inotify_helper.h"

void create_handler(struct inotify_event *event,
                    char *watched_dir,
                    char *server_url,
                    HashTable *wd_to_path,
                    HashTable *path_to_wd,
                    int ifd) {
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);

  char *src_full_path = get_src_path(base_path, event->name);
  char *dst_full_path = get_dst_path(src_full_path, watched_dir, server_url);

  printf("File url: %s\n", src_full_path);
  printf("Server url: %s\n", server_url);

  if (event->mask & IN_ISDIR) {
    copy_dir(src_full_path, dst_full_path);
    inotify_add_watch_recursively(wd_to_path, path_to_wd, ifd, src_full_path);
  } else {
    char *dst_full_path = get_dst_path(src_full_path, watched_dir, server_url);
    copy_file(src_full_path, dst_full_path);
  }

  free(dst_full_path);
}

void modify_handler(struct inotify_event *event,
                    char *watched_dir,
                    char *server_url,
                    HashTable *wd_to_path) {
  char *key = malloc(20);
  sprintf(key, "%d", event->wd);
  char *base_path = hash_table_get(wd_to_path, key);
  char *src_full_path = get_src_path(base_path, event->name);
  char *dst_full_path = get_dst_path(src_full_path, watched_dir, server_url);

  printf("File %s was modified\n", dst_full_path);

  copy_file(src_full_path, dst_full_path);
  free(dst_full_path);
}

void remove_handler(struct inotify_event *event,
                    char *watched_dir,
                    char *server_url,
                    HashTable *wd_to_path,
                    HashTable *path_to_wd,
                    int ifd) {
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