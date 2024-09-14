#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdint.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <stdbool.h>

int set_non_blocking(int fd);

typedef struct __attribute__((__packed__)) {
    uint8_t is_last_chunk;
} meta_info_t;

typedef struct __attribute__((__packed__)) {
  char file_path[PATH_MAX];
  uint32_t mode;
  uint64_t size;
  uint64_t mtime;
  uint64_t atime;
  uint64_t ctime;
} file_attrs_t;

void extract_file_metadata(char *file_path, file_attrs_t *file_attrs);
void serialize_file_attrs (file_attrs_t *file_attrs, char *buffer);
void deserialize_file_attrs (file_attrs_t *file_attrs, char *buffer);
ssize_t write_n(int fd, char *buffer, size_t size);
bool is_dir_exists(char *path);

#endif