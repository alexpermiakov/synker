#ifndef CLIENT_WATCHER_H
#define CLIENT_WATCHER_H

#include <linux/limits.h>

typedef struct {
  char watched_dir[PATH_MAX];
  char server_url[PATH_MAX];
} thread_args_t;

void *client_watcher_handler(void *args);

#endif