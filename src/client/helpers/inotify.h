#ifndef INOTIFY_HELPER_H
#define INOTIFY_HELPER_H

#include "data_structures/hash_table.h"

void inotify_add_watch_recursively(HashTable *, HashTable *, int, char *);
void inotify_remove_watch_recursively(HashTable *, int, char *);

#endif