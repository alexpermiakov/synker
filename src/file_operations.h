#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdbool.h>

void copy_file (char *src, char *dst);
void copy_dir (char *src, char *dst);
void remove_file (char *path);
void remove_dir (char *path);
bool is_dir (char *path);

#endif