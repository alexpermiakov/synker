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
#include "utils/network.h"
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
  char server_ip[16];
  int port;
  char dst_to_dir_path[PATH_MAX];

  strcpy(watched_dir, thread_args->watched_dir);
  strcpy(server_url, thread_args->server_url);
  
  extract_connection_info(server_url, server_ip, &port, dst_to_dir_path);

  int client_fd = connect_to_server(server_ip, port);
  int inotify_fd = inotify_init();

  if (inotify_fd < 0) {
    perror("inotify_init");
    exit(1);
  }

  hash_table_init(&wd_to_path);
  hash_table_init(&path_to_wd);

  inotify_add_watch_recursively(&wd_to_path, &path_to_wd, inotify_fd, watched_dir);

  // hash_table_print(&wd_to_path, print_string);
  // hash_table_print(&path_to_wd, print_int);

  int epoll_fd = epoll_init();

  epoll_add_fd(epoll_fd, inotify_fd);
  struct epoll_event events[MAX_EVENTS];

  while (1) {
    int num_events = epoll_wait_for_events(epoll_fd, events, MAX_EVENTS);

    if (num_events < 0) {
      perror("epoll_wait");
      exit(1);
    }

    struct inotify_event event_dirs[num_events]; 
    struct inotify_event event_files[num_events];
    size_t num_dirs = 0;
    size_t num_files = 0; 

    for (int i = 0; i < num_events; i++) {
      if (events[i].data.fd == inotify_fd) {
        char buf[BUFSIZ];
        size_t len = read(inotify_fd, buf, sizeof(buf));
        char *p = buf;

        while (p < buf + len) {
          struct inotify_event *event = (struct inotify_event *) p;
          printf("Event: %s, is_dir %d\n", event->name, event->mask & IN_ISDIR);

          if (event->mask & IN_ISDIR) {
            event_dirs[num_dirs] = *event;
            num_dirs++;
          } else {
            // event_files[num_files] = *event;
            // num_files++;
          }

          p += sizeof(struct inotify_event) + event->len;
        }
      }
    }

    for (size_t i = 0; i < num_dirs; i++) {
      if (event_dirs[i].mask & IN_CREATE) {
        create_handler(client_fd, &event_dirs[i], watched_dir, dst_to_dir_path, &wd_to_path, &path_to_wd, inotify_fd);
      }

      if (event_dirs[i].mask & IN_MODIFY) {
        modify_handler(client_fd, &event_dirs[i], watched_dir, dst_to_dir_path, &wd_to_path, &path_to_wd, inotify_fd);
      }

      if (event_dirs[i].mask & IN_DELETE || event_dirs[i].mask & IN_DELETE_SELF) {
        delete_handler(client_fd, &event_dirs[i], watched_dir, dst_to_dir_path, &wd_to_path, &path_to_wd, inotify_fd);
      }
    }

    for (size_t i = 0; i < num_files; i++) {
      if (event_files[i].mask & IN_CREATE) {
        create_handler(client_fd, &event_files[i], watched_dir, dst_to_dir_path, &wd_to_path, &path_to_wd, inotify_fd);
      }

      if (event_files[i].mask & IN_MODIFY) {
        modify_handler(client_fd, &event_files[i], watched_dir, dst_to_dir_path, &wd_to_path, &path_to_wd, inotify_fd);
      }

      if (event_files[i].mask & IN_DELETE || event_files[i].mask & IN_DELETE_SELF) {
        delete_handler(client_fd, &event_files[i], watched_dir, dst_to_dir_path, &wd_to_path, &path_to_wd, inotify_fd);
      }
    }
  }

  hash_table_dispose(&wd_to_path, deallocate_hash_key_values);
  hash_table_dispose(&path_to_wd, deallocate_hash_key_values);
  close(inotify_fd);
  close(epoll_fd);

  return NULL;
}