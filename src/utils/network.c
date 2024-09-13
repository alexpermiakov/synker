#include <stdio.h>
#include <linux/limits.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "network.h"
#include "file_utils.h"

void extract_connection_info(char *full_path, char *server_ip, int *port, char *file_path) {
  strcpy(server_ip, strtok(full_path, ":"));
  *port = atoi(strtok(NULL, "/"));
  sprintf(file_path, "/%s", strtok(NULL, ""));
}

int connect_to_server(char *server_url) {
  char server_ip[16];
  int port;
  char path[PATH_MAX];
  
  extract_connection_info(server_url, server_ip, &port, path);
  
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in server_addr;

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET; 
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(server_ip);

  connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  return sock_fd;
}

void add_to_epoll(int *epoll_fd, int fd) {
  if (*epoll_fd == -1) {
    perror("epoll_create1");
    close(fd);
    exit(1);
  }

  struct epoll_event event;
  event.events = EPOLLOUT | EPOLLERR; // EPOLLOUT: The associated file is available for writing
  event.data.fd = fd;

  if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
    perror("epoll_ctl");
    close(fd);
    close(*epoll_fd);
    exit(1);
  }
}
