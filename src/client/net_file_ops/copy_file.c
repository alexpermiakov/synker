#include <stdio.h>
#include <linux/limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "copy_file.h"
#include "utils/file_utils.h"
#include "utils/network.h"

void copy_file (int client_fd, char *src_file_path, char *dst_file_path) {
  int src_fd = open(src_file_path, O_RDONLY);
  
  if (src_fd == -1) {
    perror("open");
    exit(1);
  }

  ssize_t attr_size = sizeof(file_attrs_t);
  char file_attr_buffer[attr_size];
  file_attrs_t file_attrs;
  
  extract_file_metadata(src_file_path, &file_attrs);
  strcpy(file_attrs.file_path, dst_file_path);
  serialize_file_attrs(&file_attrs, file_attr_buffer);

  printf("Send file attributes\n");
  if (write_n(client_fd, file_attr_buffer, attr_size) != attr_size) {
    fprintf(stderr, "Failed to send file attributes\n");
    close(client_fd);
    exit(1);
  }

  char file_data_buffer[BUFSIZ];
  ssize_t bytes_read = 0;
  printf("Send file data\n");

  while ((bytes_read = read(src_fd, file_data_buffer, BUFSIZ)) > 0) {
    if (write(client_fd, file_data_buffer, bytes_read) < 0) {
      fprintf(stderr, "Failed to send file data\n");
      close(client_fd);
      exit(1);
    }
  }

  printf("File has sended\n\n");
}
