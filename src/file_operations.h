#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdbool.h>
#include <stdint.h>
#include <linux/limits.h>

typedef struct __attribute__((__packed__)) {
  char file_path[PATH_MAX];
  uint32_t mode;
  uint64_t size;
  uint64_t mtime;
  uint64_t atime;
  uint64_t ctime;
} file_attrs_t;

void serialize_file_attrs (file_attrs_t *file_attrs, char *buffer);
void deserialize_file_attrs (file_attrs_t *file_attrs, char *buffer);
void copy_file (char *src, char *server_url, char *postfix);
void copy_dir (char *src, char *dst);
void remove_file (char *path);
void remove_dir (char *path);
bool is_dir (char *path);

#endif