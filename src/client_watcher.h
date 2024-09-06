#ifndef CLIENT_WATCHER_H
#define CLIENT_WATCHER_H

#include <linux/limits.h>

typedef struct {
  char from_url[PATH_MAX];
  char to_url[PATH_MAX];
} thread_args_t;

void *client_watcher_handler(void *args);

#endif