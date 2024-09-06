#ifndef EVENT_HANDLERS_H
#define EVENT_HANDLERS_H

#include <sys/inotify.h>
#include "hash_table.h"

void create_handler(struct inotify_event *event,
                    char *from_url,
                    char *to_url,
                    HashTable *wd_to_path,
                    HashTable *path_to_wd,
                    int ifd);

void modify_handler(struct inotify_event *event,
                    char *from_url,
                    char *to_url,
                    HashTable *wd_to_path);
void remove_handler(struct inotify_event *event,
                    char *from_url,
                    char *to_url,
                    HashTable *wd_to_path,
                    HashTable *path_to_wd,
                    int ifd);

#endif