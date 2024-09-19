#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "delete_dir.h"
#include "delete_file.h"
#include "utils/file_utils.h"

void delete_dir(int client_fd, char *dst_file_path) {
  ssize_t attr_size = sizeof(file_attrs_t);
  char file_attr_buffer[attr_size];
  file_attrs_t file_attrs;
  
  strcpy(file_attrs.file_path, dst_file_path);
  file_attrs.operation = DELETE;
  serialize_file_attrs(&file_attrs, file_attr_buffer);

  printf("Remove dir %s\n", dst_file_path);
  
  if (write(client_fd, file_attr_buffer, attr_size) < 0) {
    fprintf(stderr, "Failed to remove dir\n");
    close(client_fd);
    exit(1);
  }
}