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

// TODO: extract metadata to a struct
typedef struct {
  int client_fd;
  int fd;
  char buffer[BUFSIZ];
  size_t total_read;
  file_attrs_t *file_attrs;
} client_t;

client_t *create_client(int client_fd) {
  client_t *client = (client_t *) malloc(sizeof(client_t));
  if (client == NULL) {
    perror("malloc");
    return NULL;
  }

  client->client_fd = client_fd;
  client->fd = -1;
  client->total_read = 0;
  client->file_attrs = NULL;
  memset(client->buffer, 0, BUFSIZ);

  return client;
}

ssize_t read_file_attrs(client_t *client) {
  int n = read(client->client_fd, client->buffer + client->total_read, BUFSIZ - client->total_read);

  if (n == 0) {
    printf("Connection closed\n\n");
    close(client->client_fd);
    return -1;
  }

  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // no more data to read right now
      return -1;
    } else {
      perror("read");
      close(client->client_fd);
      return -1;
    }
  }

  client->total_read += n;
  size_t metadata_size = sizeof(file_attrs_t);

  if (client->total_read < metadata_size) {
    return 0;
  }

  file_attrs_t *file_attrs = (file_attrs_t *) malloc(sizeof(file_attrs_t));
  deserialize_file_attrs(file_attrs, client->buffer);

  printf("Received file attributes\n");
  printf("File path: %s\n", file_attrs->file_path);
  printf("Mode: %d\n", file_attrs->mode);
  printf("Size: %lu\n", file_attrs->size);
  printf("Mtime: %lu\n", file_attrs->mtime);
  printf("Atime: %lu\n", file_attrs->atime);
  printf("Ctime: %lu\n", file_attrs->ctime);

  client->file_attrs = file_attrs;

  if (file_attrs->mode & S_IFDIR) {
    printf("Creating directory %s\n", file_attrs->file_path);
    mkdir(file_attrs->file_path, file_attrs->mode & 0777);
  } else {
    printf("Creating file %s and with %d mode\n", file_attrs->file_path, file_attrs->mode & 0777);
    client->fd = open(file_attrs->file_path, O_CREAT | O_WRONLY, file_attrs->mode & 0777);
    
    if (client->fd < 0) {
      perror("open");
      return -1;
    }

    if (write_all(client->fd, client->buffer + metadata_size, client->total_read - metadata_size) == -1) {
      perror("write_all");
      return -1;
    }
  }

  return 0;
}

ssize_t read_file_data(client_t *client) {
  int n = read(client->client_fd, client->buffer, BUFSIZ);

  if (n == 0) {
    printf("Connection closed\n\n");
    close(client->client_fd);
    return -1;
  }

  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // no more data to read right now
      return -1;
    } else {
      perror("read");
      close(client->client_fd);
      return -1;
    }
  }

  client->total_read += n;
  
  if (write_all(client->fd, client->buffer, n) == -1) {
    perror("write_all");
    return -1;
  }

  if (client->total_read == sizeof(file_attrs_t) + client->file_attrs->size) {
    printf("File received\n\n");
    free(client->file_attrs);
    close(client->fd);
    return 0;
  }

  return 0;
}

int handle_client(client_t *client) {
  while (1) {
    if (client->file_attrs == NULL) {
      if (read_file_attrs(client) == -1) {
        return -1;
      }
    } else {
      if (read_file_data(client) == -1) {
        return -1;
      }
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
  client_t *clients[1024];

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

          client_t *client = create_client(client_fd);
          if (client == NULL) {
            continue;
          }

          event.events = EPOLLIN;
          event.data.fd = client_fd;
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
            perror("epoll_ctl");
            continue;
          }

          clients[client_fd] = client;

          printf("New connection from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        }
      } else {
        int client_fd = events[i].data.fd;
        client_t *client = clients[client_fd];
        int result = handle_client(client);

        if (result == 1) {
          // full message received
          close(client_fd);
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
          free(client);
          clients[client_fd] = NULL;
        } else if (result == -1) {
            // client disconnected or error
            close(client_fd);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            free(client);
            clients[client_fd] = NULL;
        }
      }
    }
  }

  close(server_fd);

  return NULL;
}