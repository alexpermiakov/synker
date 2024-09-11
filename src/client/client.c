#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <linux/limits.h>
#include <signal.h>

#include "data_structures/hash_table.h"
#include "utils/hash_table_utils.h"
#include "utils/string_utils.h"
#include "client/helpers/epoll.h"
#include "client/helpers/inotify.h"
#include "client/event_handlers.h"
#include "client/client.h"

void *client(void *args) {
  HashTable wd_to_path;
  HashTable path_to_wd;
  thread_args_t *thread_args = (thread_args_t *) args;

  char watched_dir[PATH_MAX];
  char server_url[PATH_MAX];

  strcpy(watched_dir, thread_args->watched_dir);
  strcpy(server_url, thread_args->server_url);

  int ifd = inotify_init();

  if (ifd < 0) {
    perror("inotify_init");
    exit(1);
  }

  hash_table_init(&wd_to_path);
  hash_table_init(&path_to_wd);
  inotify_add_watch_recursively(&wd_to_path, &path_to_wd, ifd, watched_dir);
  // copy_dir(watched_dir, server_url);

  // hash_table_print(&wd_to_path, print_string);
  // hash_table_print(&path_to_wd, print_int);

  int epoll_fd = epoll_init();
  epoll_add_fd(epoll_fd, ifd);
  struct epoll_event events[MAX_EVENTS];

  printf("Copying from %s to %s:\n", watched_dir, server_url);

  while (1) {
    int num_events = epoll_wait_for_events(epoll_fd, events, MAX_EVENTS);

    if (num_events < 0) {
      perror("epoll_wait");
      exit(1);
    }

    for (int i = 0; i < num_events; i++) {
      if (events[i].data.fd == ifd) {
        char buf[BUFSIZ];
        ssize_t len = read(ifd, buf, sizeof(buf));
        char *p = buf;

        while (p < buf + len) {
          struct inotify_event *event = (struct inotify_event *) p;

          if (event->mask & IN_CREATE) {
            create_handler(event, watched_dir, server_url, &wd_to_path, &path_to_wd, ifd);            
          }

          if (event->mask & IN_MODIFY) {
            modify_handler(event, watched_dir, server_url, &wd_to_path, &path_to_wd, ifd);
          }

          if (event->mask & IN_DELETE || event->mask & IN_DELETE_SELF) {
            remove_handler(event, watched_dir, server_url, &wd_to_path, &path_to_wd, ifd);
          }
          
          p += sizeof(struct inotify_event) + event->len;
        }
      }
    }
  }

  hash_table_dispose(&wd_to_path, deallocate_hash_key_values);
  hash_table_dispose(&path_to_wd, deallocate_hash_key_values);
  close(ifd);
  close(epoll_fd);

  return NULL;
}