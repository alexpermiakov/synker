#include <stdio.h>
#include <linux/limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "copy_file.h"
#include "utils/file_utils.h"

#define MAX_EVENTS 1 // We only have one connection to handle

void copy_file (char *src_full_path, char *dst_full_path) {
  int src_fd = open(src_full_path, O_RDONLY);
  
  if (src_fd == -1) {
    perror("open");
    exit(1);
  }

  printf("Full URL: %s\n", dst_full_path);

  // TODO: extract this to a function
  char *domain_name = strtok(dst_full_path, ":");
  char *port = strtok(NULL, "/");
  char *path_without_slash = strtok(NULL, "");
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "/%s", path_without_slash);

  printf("Server name: %s\n", domain_name);
  printf("Server Port: %s\n", port);
  printf("File Path: %s\n", path);

  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    perror("socket");
    exit(1);
  }

  int flags = fcntl(sock_fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl");
    exit(1);
  }

  if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl");
    exit(1);
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET; 
  server_addr.sin_port = htons(atoi(port));
  server_addr.sin_addr.s_addr = inet_addr(domain_name);

  connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    close(sock_fd);
    exit(1);
  }

  struct epoll_event event;
  event.events = EPOLLOUT | EPOLLERR; // EPOLLOUT: The associated file is available for writing
  event.data.fd = sock_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event) == -1) {
    perror("epoll_ctl");
    close(sock_fd);
    close(epoll_fd);
    exit(1);
  }

  struct epoll_event events[MAX_EVENTS];
  int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 10000); // 10 seconds to wait
  if (n == -1) {
    perror("epoll_wait");
    close(sock_fd);
    close(epoll_fd);
    exit(1);
  } else if (n == 0) {
    fprintf(stderr, "Timeout\n");
    close(sock_fd);
    close(epoll_fd);
    exit(1);
  }

  if (events[0].events & EPOLLERR) {
    fprintf(stderr, "Error or Hangup\n");
    close(sock_fd);
    close(epoll_fd);
    exit(1);
  }

  if (events[0].events & EPOLLOUT) {
    int error = 0;
    socklen_t len = sizeof(error);

    if (getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
      perror("getsockopt");
      close(sock_fd);
      close(epoll_fd);
      exit(1);
    }

    printf("Connection successful\n");
  }
  
  // TODO: extract this to a function
  file_attrs_t file_attrs;
  struct stat info;

  if (stat(src_full_path, &info) == -1) {
    perror("fstat");
    close(sock_fd);
    close(epoll_fd);
    exit(1);
  }

  strcpy(file_attrs.file_path, path);
  file_attrs.size = info.st_size;
  file_attrs.mode = info.st_mode;
  file_attrs.mtime = info.st_mtime;
  file_attrs.atime = info.st_atime;
  file_attrs.ctime = info.st_ctime;

  char buffer[BUFSIZ];
  size_t expected_size = sizeof(file_attrs_t);

  serialize_file_attrs(&file_attrs, buffer);

  if (write_all(sock_fd, buffer, expected_size) != expected_size) {
    fprintf(stderr, "Failed to send file attributes\n");
    close(sock_fd);
    close(epoll_fd);
    exit(1);
  }

  close(sock_fd);
  close(epoll_fd);
}
