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

void copy_file (int sock_fd, char *src_file_path, char *dst_file_path) {
  int src_fd = open(src_file_path, O_RDONLY);
  
  if (src_fd == -1) {
    perror("open");
    exit(1);
  }

  char buffer[BUFSIZ];
  file_attrs_t file_attrs;
  ssize_t bytes_read;

  extract_file_metadata(src_file_path, &file_attrs);
  strcpy(file_attrs.file_path, dst_file_path);
  serialize_file_attrs(&file_attrs, buffer);

  printf("Send file attributes\n");
  if (write_all(sock_fd, buffer, sizeof(file_attrs_t)) == -1) {
    fprintf(stderr, "Failed to send file attributes\n");
    close(sock_fd);
    exit(1);
  }

  printf("Send file data\n");
  while ((bytes_read = read(src_fd, buffer, BUFSIZ)) > 0) {
    if (write_all(sock_fd, buffer, bytes_read) == -1) {
      fprintf(stderr, "Failed to send file data\n");
      close(sock_fd);
      exit(1);
    }
  }

  printf("File has sended\n\n");
}
