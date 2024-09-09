#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdbool.h>
#include <stdint.h>
#include <linux/limits.h>

void copy_file (char *src_full_path, char *dst_full_path);
void copy_dir (char *src, char *dst);
void remove_file (char *path);
void remove_dir (char *path);

#endif