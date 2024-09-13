#ifndef EVENT_HANDLERS_H
#define EVENT_HANDLERS_H

#include <sys/inotify.h>
#include "data_structures/hash_table.h"

void create_handler(int sock_fd, struct inotify_event *event, char *watched_dir, char *dst_to_dir_path, HashTable *wd_to_path, HashTable *path_to_wd, int inotify_fd);
void modify_handler(int sock_fd, struct inotify_event *event, char *watched_dir, char *dst_to_dir_path, HashTable *wd_to_path, HashTable *path_to_wd, int inotify_fd);
void remove_handler(int sock_fd, struct inotify_event *event, char *watched_dir, char *dst_to_dir_path, HashTable *wd_to_path, HashTable *path_to_wd, int inotify_fd);

#endif