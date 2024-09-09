#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "client/client.h"
#include "server/server.h"

int main (int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <watched_dir> <server_url>\n", argv[0]);
    return 1;
  }

  thread_args_t thread_args;
  strcpy(thread_args.watched_dir, argv[1]);
  strcpy(thread_args.server_url, argv[2]);

  pthread_t client_thread, server_thread;
  
  pthread_create(&client_thread, NULL, client, &thread_args);
  pthread_create(&server_thread, NULL, server, thread_args.watched_dir);
  pthread_join(client_thread, NULL);
  pthread_join(server_thread, NULL);

  return 0;
}