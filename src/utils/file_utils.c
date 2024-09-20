#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#include "file_utils.h"

int remove_dir(char *path) {
  DIR *dir = opendir(path);

  if (dir == NULL) {
    perror("opendir");
    return -1;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s/%s", path, entry->d_name);

    struct stat st;
    if (stat(full_path, &st) < 0) {
      perror("stat");
      return -1;
    }

    if (S_ISDIR(st.st_mode)) {
      remove_dir(full_path);
    } else {
      if (remove(full_path) < 0) {
        perror("remove");
        return -1;
      }
    }
  }

  closedir(dir);

  if (remove(path) < 0) {
    perror("remove");
    return -1;
  }

  return 0;
}

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
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}