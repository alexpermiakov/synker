#ifndef EVENT_HANDLERS_H
#define EVENT_HANDLERS_H

#include <sys/inotify.h>
#include "data_structures/hash_table.h"

void create_handler(int sock_fd, struct inotify_event *event, char *watched_dir, HashTable *wd_to_path, HashTable *path_to_wd, int ifd);
void modify_handler(int sock_fd, struct inotify_event *event, char *watched_dir, HashTable *wd_to_path, HashTable *path_to_wd, int ifd);
void remove_handler(int sock_fd, struct inotify_event *event, char *watched_dir, HashTable *wd_to_path, HashTable *path_to_wd, int ifd);

#endif