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

#define MAX_EVENTS 64 // maximum number of events to be returned by epoll_wait
#define PORT 80

typedef enum {
  READING_FILE_ATTRS,
  READING_FILE_DATA,
  WRITING_RESPONSE
} connection_state_t;

typedef struct {
  int fd;                     // client socket file descriptor
  connection_state_t state;   // current state of the connection
  char buffer[BUFSIZ];        // buffer for reading/writing data
  size_t buffer_size;         // size of the buffer
  size_t total_read;          // total bytes read so far
  size_t total_written;       // total bytes written so far
  size_t expected_size;       // expected total size of the data to be read
  file_attrs_t file_attrs;    // file attributes
  int file_fd;                // file descriptor for the file being written
} connection_t;

int handle_client(connection_t *conn) {
  ssize_t result = 0;

  while (1) {
    if (conn->state == READING_FILE_ATTRS) {
      size_t attr_size = sizeof(file_attrs_t);
      result = read(conn->fd, conn->buffer, attr_size);
      
      if (result < 0) {
        perror("read_n");
        close(conn->fd);
        return -1;
      }

      conn->total_read += result;

      if (conn->total_read == attr_size) {
        deserialize_file_attrs(&conn->file_attrs, conn->buffer);
        conn->expected_size = conn->file_attrs.size;
        conn->total_read = 0;
        conn->file_fd = open(conn->file_attrs.file_path, O_CREAT | O_WRONLY | O_TRUNC, conn->file_attrs.mode & 0777);

        if (conn->fd < 0) {
          perror("open");
          close(conn->fd);
          return -1;
        }

        conn->state = READING_FILE_DATA;
      } else {
        break; // wait for more data
      }
    }

    if (conn->state == READING_FILE_DATA) {
      while (conn->total_read < conn->expected_size) {
        size_t to_read = BUFSIZ;
        if (conn->expected_size - conn->total_read < BUFSIZ) {
          to_read = conn->expected_size - conn->total_read;
        }

        result = read(conn->fd, conn->buffer, to_read);

        if (result > 0) {
          ssize_t bytes_written = 0;
          ssize_t total_written = 0;

          while(total_written < result) {
            bytes_written = write(conn->file_fd, conn->buffer + total_written, result - total_written);

            if (bytes_written > 0) {
              total_written += bytes_written;
            } else if (bytes_written == -1 && errno == EINTR) {
              continue;
            } else if (bytes_written == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
              break;
            } else {
              perror("write");
              close(conn->fd);
              return -1;
            }
          }

          conn->total_read += result;
        } else if (result == 0) {
          perror("read");
        } else {
          if (errno == EINTR) {
            continue;
          } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
          } else {
            perror("read");
            close(conn->fd);
            return -1;
          }
        }
      }

      if (conn->total_read == conn->expected_size) {
        conn->state = READING_FILE_ATTRS;
        conn->total_read = 0;
        conn->expected_size = sizeof(file_attrs_t);
      } else {
        break;
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

  if (set_non_blocking(server_fd) < 0) {
    perror("set_non_blocking");
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
  event.events = EPOLLIN | EPOLLET; // edge-triggered, meaning that we will be notified only once when new data is available
  event.data.fd = server_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
    perror("epoll_ctl");
    close(server_fd);
    close(epoll_fd);
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
      if (events[i].events & (EPOLLERR | EPOLLHUP)) {
        perror("epoll_wait");
        close(events[i].data.fd);
        continue;
      }

      // handle incoming connection
      if (events[i].data.fd == server_fd) {
        while(1) {
          socklen_t address_len = sizeof(address);
          int client_fd = accept(server_fd, (struct sockaddr *) &address, &address_len);

          if (client_fd < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) { // no more connections
              break;
            } else {
              perror("accept");
              return NULL;
            }
          }

          if (strcmp(inet_ntoa(address.sin_addr), "136.244.98.188") &&
              strcmp(inet_ntoa(address.sin_addr), "95.179.200.237")) {
            printf("Rejected connection from %s\n", inet_ntoa(address.sin_addr));
            close(client_fd);
            continue;
          }

          printf("Accepted connection\n");
          
          if (set_non_blocking(client_fd) < 0) {
            perror("set_non_blocking");
            close(client_fd);
            continue;
          }

          connection_t *connection = (connection_t *) malloc(sizeof(connection_t));
          connection->fd = client_fd;
          connection->state = READING_FILE_ATTRS;
          connection->expected_size = sizeof(file_attrs_t);
          connection->file_fd = -1;

          event.events = EPOLLIN | EPOLLET;
          event.data.ptr = connection;

          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
            perror("epoll_ctl");
            close(client_fd);
            continue;
          }

          printf("New connection from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        }
      } else { // later, handle incoming data
        connection_t *connection = (connection_t *) events[i].data.ptr;
        if (handle_client(connection) < 0) {
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connection->fd, NULL);
          close(connection->fd);
          close(epoll_fd);
          free(connection);
        }
      }
    }
  }

  close(server_fd);

  return NULL;
}