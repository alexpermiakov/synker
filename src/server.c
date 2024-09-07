#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "server.h"

#define MAX_EVENTS 10
#define PORT 5551

// TODO: extract metadata to a struct
typedef struct {
  int client_fd;
  size_t expected_data_size;
  int file_name_length;
  char *file_name;
  size_t total_read;
  int fd;
  char buffer[BUFSIZ];
} client_t;

client_t *create_client(int client_fd) {
  client_t *client = (client_t *) malloc(sizeof(client_t));
  if (client == NULL) {
    perror("malloc");
    return NULL;
  }

  client->client_fd = client_fd;
  client->expected_data_size = 0;
  client->file_name_length = 0;
  client->file_name = NULL;
  client->total_read = 0;
  client->fd = -1;
  memset(client->buffer, 0, BUFSIZ);

  return client;
}

int handle_client(client_t *client) {
  while (1) {
    int n = read(client->client_fd, client->buffer, BUFSIZ);
    
    if (n == 0) {
      printf("Connection closed\n");
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

    if (client->expected_data_size == 0 && client->total_read >= sizeof(int)) {
      // assume first 4 bytes is the size of the data
      client->expected_data_size = *(int *) client->buffer;
      printf("Expecting %lu bytes\n", client->expected_data_size);
    }

    if (client->file_name_length == 0 && client->total_read >= sizeof(int) + sizeof(int)) {
      // assume next 4 bytes is the length of the file name
      client->file_name_length = *(int *) (client->buffer + sizeof(int));
      printf("Expecting file name of length %d\n", client->file_name_length);
    }

    size_t full_metadata_size = sizeof(int) + sizeof(int) + client->file_name_length;

    if (client->file_name == NULL && client->total_read >= full_metadata_size) {
      client->file_name = malloc(client->file_name_length + 1);
      memcpy(client->file_name, client->buffer + sizeof(int) + sizeof(int), client->file_name_length);
      client->file_name[client->file_name_length] = '\0';
      printf("Received file name: %s\n", client->file_name);

      client->fd = open(client->file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
      if (client->fd < 0) {
        perror("open");
        close(client->client_fd);
        return -1;
      }

      write(client->fd, client->buffer + full_metadata_size, client->total_read - full_metadata_size);
    } else if (client->file_name != NULL) {
      write(client->fd, client->buffer, n);
    }

    if (client->total_read >= client->expected_data_size) {
      printf("Received %lu bytes: %s\n", client->expected_data_size, client->buffer + sizeof(int));
      client->expected_data_size = 0;
      client->total_read = 0;

      // print message and close connection
      printf("%s\n", client->buffer + sizeof(int));
      close(client->client_fd);

      return 1;
    }
  }

  return 0;
}

void *server_handler (void *args) {
  char *url = (char *) args;
  printf("Server, add incomming changes to %s directory\n", url);

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
    int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
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