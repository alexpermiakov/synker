#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "client_watcher.h"
#include "server.h"

int main (int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <watched_dir> <server_url>\n", argv[0]);
    return 1;
  }

  thread_args_t thread_args;
  strcpy(thread_args.watched_dir, argv[1]);
  strcpy(thread_args.server_url, argv[2]);

  pthread_t client_watcher, server;
  
  pthread_create(&client_watcher, NULL, client_watcher_handler, &thread_args);
  pthread_create(&server, NULL, server_handler, thread_args.watched_dir);
  pthread_join(client_watcher, NULL);
  pthread_join(server, NULL);

  return 0;
}