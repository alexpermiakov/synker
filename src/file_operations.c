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

#include "file_operations.h"

#define MAX_EVENTS 1 // We only have one connection to handle

bool is_dir_exists(char *path) {
  struct stat info;
  
  if (lstat(path, &info)) {
    return 0;
  }

  return info.st_mode & S_IFDIR;
}

void copy_file (char *src, char *server_url, char *postfix) {
  int src_fd = open(src, O_RDONLY);
  
  if (src_fd == -1) {
    perror("open");
    exit(1);
  }

  char *full_url = malloc(strlen(server_url) + strlen(postfix) + 1);
  strcpy(full_url, server_url);
  strcat(full_url, postfix);

  char *domain_name = strtok(full_url, ":");
  char *port = strtok(NULL, "/");
  char *path_without_slash = strtok(NULL, "");
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "/%s", path_without_slash);

  printf("Server name: %s\n", domain_name);
  printf("Server Port: %s\n", port);
  printf("File Path: %s\n", path);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket");
    exit(1);
  }

  int flags = fcntl(sock, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl");
    exit(1);
  }

  if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl");
    exit(1);
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET; // IPv4
  server_addr.sin_port = htons(atoi(port));
  server_addr.sin_addr.s_addr = inet_addr(domain_name);

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    perror("connect");
    exit(1);
  }

  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    close(sock);
    exit(1);
  }

  struct epoll_event event;
  event.events = EPOLLOUT | EPOLLERR; // EPOLLOUT: The associated file is available for writing
  event.data.fd = sock;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &event) == -1) {
    perror("epoll_ctl");
    close(sock);
    close(epoll_fd);
    exit(1);
  }

  struct epoll_event events[MAX_EVENTS];
  int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 10000); // 10 seconds to wait
  if (n == -1) {
    perror("epoll_wait");
    close(sock);
    close(epoll_fd);
    exit(1);
  } else if (n == 0) {
    fprintf(stderr, "Timeout\n");
    close(sock);
    close(epoll_fd);
    exit(1);
  }

  if (events[0].events & EPOLLERR) {
    fprintf(stderr, "Error or Hangup\n");
    close(sock);
    close(epoll_fd);
    exit(1);
  }

  if (events[0].events & EPOLLOUT) {
    // check if the connection was successful
    int error = 0;
    socklen_t len = sizeof(error);

    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
      perror("getsockopt");
      close(sock);
      close(epoll_fd);
      exit(1);
    }

    printf("Connection successful\n");
  }

  int filename_len = strlen(path);
  char *filename = path;
  int expected_size = sizeof(int) + sizeof(int) + filename_len;
  char buffer[BUFSIZ];

  memcpy(buffer, &expected_size, sizeof(int));
  memcpy(buffer + sizeof(int), &filename_len, sizeof(int));
  memcpy(buffer + sizeof(int) + sizeof(int), filename, filename_len);

  int total_written = 0;

  while (total_written < expected_size) {
    int n = write(sock, buffer + total_written, expected_size - total_written);

    if (n == -1) {
      perror("write");
      close(sock);
      close(epoll_fd);
      exit(1);
    }

    total_written += n;
  }

  printf("Sent file name\n");

  close(sock);
  close(epoll_fd);
}

void copy_dir (char *src, char *dst) {
  if (!is_dir_exists(dst) && mkdir(dst, 0777) == -1) {
    perror("mkdir");
    exit(1);
  }

  DIR *dir = opendir(src);

  if (dir == NULL) {
    perror("opendir");
    exit(1);
  }

  struct dirent *entry;

  while((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char src_path[PATH_MAX];
    char dst_path[PATH_MAX];

    snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
    snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

    if (entry->d_type == DT_DIR) {
      copy_dir(src_path, dst_path);
    } else {
      copy_file(src_path, dst_path, dst_path);
    }
  }

  closedir(dir);
}

void remove_file(char *path) {
  if (unlink(path) == -1) {
    perror("unlink");
    exit(1);
  }
}

void remove_dir(char *path) {
  DIR *dir = opendir(path);

  if (dir == NULL) {
    perror("opendir");
    exit(1);
  }

  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

    if (entry->d_type == DT_DIR) {
      remove_dir(full_path);
    } else {
      remove_file(full_path);
    }
  }

  closedir(dir);

  if (rmdir(path) == -1) {
    perror("rmdir");
    exit(1);
  }
}