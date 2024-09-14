#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "server.h"
#include "utils/file_utils.h"

#define MAX_EVENTS 10
#define PORT 80

size_t read_file_attrs(int client_fd, file_attrs_t *file_attrs) {
  size_t attr_size = sizeof(file_attrs_t);
  char buffer[attr_size];
  
  size_t n = read_n(client_fd, buffer, attr_size);

  if (n == -1lu || n < attr_size) {
    perror("read");
    close(client_fd);
    return -1;
  }

  deserialize_file_attrs(file_attrs, buffer);

  printf("Received file attributes\n");
  printf("File path: %s\n", file_attrs->file_path);
  printf("Mode: %d\n", file_attrs->mode);
  printf("Size: %lu\n", file_attrs->size);
  printf("Mtime: %lu\n", file_attrs->mtime);
  printf("Atime: %lu\n", file_attrs->atime);
  printf("Ctime: %lu\n", file_attrs->ctime);

  if (file_attrs->mode & S_IFDIR) {
    mkdir(file_attrs->file_path, file_attrs->mode & 0777);
  } else {
    if (open(file_attrs->file_path, O_CREAT | O_WRONLY | O_APPEND, file_attrs->mode & 0777) < 0) {
      perror("open");
      return -1;
    }
  }

  return 1;
}

size_t read_file_data(int client_fd, file_attrs_t *file_attrs) {
  char buffer[BUFSIZ];
  size_t n = read_n(client_fd, buffer, BUFSIZ);

  if (n == -1lu) {
    perror("read");
    close(client_fd);
    return -1;
  }

  int fd = open(file_attrs->file_path, O_CREAT | O_WRONLY | O_APPEND, file_attrs->mode & 0777);

  if (write_n(fd, buffer, n) != n) {
    perror("write_n");
    return -1;
  }

  size_t file_size = lseek(fd, 0, SEEK_END);
  if (file_size == -1lu) {
    perror("lseek");
    return -1;
  }

  printf("file_attrs->size %lu\n", file_attrs->size);
  printf("file_size %lu\n", file_size);

  if (file_attrs->size == file_size) {
    printf("File received\n\n");
    close(fd);
    return 1;
  }

  return 0;
}

int handle_client(int client_fd) {
  while (1) {
    file_attrs_t file_attrs;
    size_t res_attrs = read_file_attrs(client_fd, &file_attrs);

    if (res_attrs == -1lu) {
      return -1;
    }
    
    size_t res_data = read_file_data(client_fd, &file_attrs);
    if (res_data == -1lu || res_data == 1lu) {
      return res_data;
    }
  }

  return 0;
}

void *server () {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
    return NULL;
  }

  int flags = fcntl(server_fd, F_GETFL, 0);
  if(fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    perror("fcntl");
    return NULL;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
    perror("bind");
    return NULL;
  }

  if (listen(server_fd, 10) < 0) {
    perror("listen");
    return NULL;
  }

  printf("Server listening on port %d\n", PORT);

  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    perror("epoll_create1");
    return NULL;
  }

  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = server_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
    perror("epoll_ctl");
    return NULL;
  }

  struct epoll_event events[MAX_EVENTS];

  while (1) {
    printf("Waiting for events\n\n");
    int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    printf("Received %d events\n", num_events);
    
    if (num_events < 0) {
      perror("epoll_wait");
      return NULL;
    }

    for (int i = 0; i < num_events; i++) {
      // incoming connection
      if (events[i].data.fd == server_fd) {
        while(1) {
          socklen_t address_len = sizeof(address);
          int client_fd = accept(server_fd, (struct sockaddr *) &address, &address_len);

          if (client_fd < 0) {
            break;
          }

          if (strcmp(inet_ntoa(address.sin_addr), "136.244.98.188") &&
              strcmp(inet_ntoa(address.sin_addr), "95.179.200.237")) {
            printf("Rejected connection from %s\n", inet_ntoa(address.sin_addr));
            close(client_fd);
            continue;
          }

          printf("Accepted connection\n");

          int flags = fcntl(client_fd, F_GETFL, 0);
          if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            perror("fcntl");
            continue;
          }

          event.events = EPOLLIN;
          event.data.fd = client_fd;
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
            perror("epoll_ctl");
            continue;
          }

          printf("New connection from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        }
      } else {
        int client_fd = events[i].data.fd;
        printf("Received data from %d client\n", client_fd);
        handle_client(client_fd);
      }
    }
  }

  close(server_fd);

  return NULL;
}