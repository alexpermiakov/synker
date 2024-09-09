#ifndef client_H
#define client_H

#include <linux/limits.h>

typedef struct {
  char watched_dir[PATH_MAX];
  char server_url[PATH_MAX];
} thread_args_t;

void *client(void *args);

#endif