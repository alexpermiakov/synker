#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdint.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <stdbool.h>

int set_non_blocking(int fd);

typedef struct __attribute__((__packed__)) {
  char file_path[PATH_MAX];
  uint32_t mode;
  uint64_t size;
  uint8_t operation;
} file_attrs_t;

typedef enum {
  CREATE_FILE = 1,
  DELETE,
  CREATE_DIR,
  MODIFY
} operation_t;

int remove_dir(char *path);
void extract_file_metadata(char *file_path, file_attrs_t *file_attrs);
void serialize_file_attrs (file_attrs_t *file_attrs, char *buffer);
void deserialize_file_attrs (file_attrs_t *file_attrs, char *buffer);
bool is_dir_exists(char *path);
#endif