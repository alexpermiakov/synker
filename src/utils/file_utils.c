#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "file_utils.h"

void extract_file_metadata(char *file_path, file_attrs_t *file_attrs) {
  struct stat info;

  if (stat(file_path, &info) == -1) {
    perror("fstat");
    exit(1);
  }

  strcpy(file_attrs->file_path, file_path);
  file_attrs->size = info.st_size;
  file_attrs->mode = info.st_mode;
  file_attrs->mtime = info.st_mtime;
  file_attrs->atime = info.st_atime;
  file_attrs->ctime = info.st_ctime;
}

void serialize_file_attrs (file_attrs_t *file_attrs, char *buffer) {
  memcpy(buffer, file_attrs->file_path, PATH_MAX);
  memcpy(buffer + PATH_MAX, &file_attrs->mode, sizeof(uint32_t));
  memcpy(buffer + PATH_MAX + sizeof(uint32_t), &file_attrs->size, sizeof(uint64_t));
  memcpy(buffer + PATH_MAX + sizeof(uint32_t) + sizeof(uint64_t), &file_attrs->mtime, sizeof(uint64_t));
  memcpy(buffer + PATH_MAX + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t), &file_attrs->atime, sizeof(uint64_t));
  memcpy(buffer + PATH_MAX + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint64_t), &file_attrs->ctime, sizeof(uint64_t));
}

void deserialize_file_attrs (file_attrs_t *file_attrs, char *buffer) {
  memcpy(file_attrs->file_path, buffer, PATH_MAX);
  memcpy(&file_attrs->mode, buffer + PATH_MAX, sizeof(uint32_t));
  memcpy(&file_attrs->size, buffer + PATH_MAX + sizeof(uint32_t), sizeof(uint64_t));
  memcpy(&file_attrs->mtime, buffer + PATH_MAX + sizeof(uint32_t) + sizeof(uint64_t), sizeof(uint64_t));
  memcpy(&file_attrs->atime, buffer + PATH_MAX + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t), sizeof(uint64_t));
  memcpy(&file_attrs->ctime, buffer + PATH_MAX + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint64_t), sizeof(uint64_t));
}

size_t write_all(int fd, char *buffer, size_t size) {
  size_t total_written = 0;

  while (total_written < size) {
    int n = write(fd, buffer + total_written, size - total_written);

    if (n == -1) {
      perror("write");
      exit(1);
    }

    total_written += n;
  }

  return total_written;
}

bool is_dir_exists(char *path) {
  struct stat info;
  
  if (lstat(path, &info)) {
    return 0;
  }

  return info.st_mode & S_IFDIR;
}