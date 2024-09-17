#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

#include "file_utils.h"

int set_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl");
    return -1;
  }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl");
    return -1;
  }

  return 0;
}

void extract_file_metadata(char *file_path, file_attrs_t *file_attrs) {
  struct stat info;

  if (stat(file_path, &info) == -1) {
    perror("fstat");
    exit(1);
  }

  file_attrs->size = info.st_size;
  file_attrs->mode = info.st_mode;
}

void serialize_file_attrs (file_attrs_t *file_attrs, char *buffer) {
  memcpy(buffer, file_attrs->file_path, PATH_MAX);
  memcpy(buffer + PATH_MAX, &file_attrs->mode, sizeof(uint32_t));
  memcpy(buffer + PATH_MAX + sizeof(uint32_t), &file_attrs->size, sizeof(uint64_t));
  memcpy(buffer + PATH_MAX + sizeof(uint32_t) + sizeof(uint64_t), &file_attrs->operation, sizeof(uint8_t));
}

void deserialize_file_attrs (file_attrs_t *file_attrs, char *buffer) {
  memcpy(file_attrs->file_path, buffer, PATH_MAX);
  memcpy(&file_attrs->mode, buffer + PATH_MAX, sizeof(uint32_t));
  memcpy(&file_attrs->size, buffer + PATH_MAX + sizeof(uint32_t), sizeof(uint64_t));
  memcpy(&file_attrs->operation, buffer + PATH_MAX + sizeof(uint32_t) + sizeof(uint64_t), sizeof(uint8_t));
}

bool is_dir_exists(char *path) {
  struct stat info;
  
  if (lstat(path, &info)) {
    return 0;
  }

  return info.st_mode & S_IFDIR;
}

ssize_t read_n(int fd, char *buffer, ssize_t size) {
  ssize_t total = 0;
  
  while (total < size) {
    ssize_t n = read(fd, buffer + total, size - total);

    if (n > 0) {
      total += n;
    } else if (n == -1 && errno == EINTR) {
      continue;
    } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      break;
    } else {
      perror("read");
      return -1;
    }
  }

  return total;
}

ssize_t write_n(int fd, char *buffer, ssize_t size) {
  ssize_t total = 0;

  while(total < size) {
    ssize_t n = write(fd, buffer + total, size - total);

    if (n > 0) {
      total += n;
    } else if (n == -1 && errno == EINTR) {
      continue;
    } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      break;
    } else {
      perror("write");
      return -1;
    }
  }

  return total;
}