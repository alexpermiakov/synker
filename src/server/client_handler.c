#include <stdio.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "utils/file_utils.h"
#include "client_handler.h"

int handle_client(connection_t *conn) {
  while (1) {
    printf("Reading data from client\n");
    printf("State: %d\n", conn->state);

    if (conn->state == READING_FILE_ATTRS) {
      size_t attr_size = sizeof(file_attrs_t);
      ssize_t n = read(conn->fd, conn->buffer + conn->total_read, attr_size - conn->total_read);
      
      if (n < 0) {
        break;
      }

      conn->total_read += n;

      printf("Read %zd bytes\n", n);
      printf("attr_size: %zu bytes\n", attr_size);
      printf("total_read: %zu bytes\n", conn->total_read);

      if (conn->total_read < attr_size) {
        break;
      }

      deserialize_file_attrs(&conn->file_attrs, conn->buffer);
      
      printf("Received file attributes\n");
      printf("File path: %s\n", conn->file_attrs.file_path);
      printf("File operation: %hhu\n", conn->file_attrs.operation);

      if (conn->file_attrs.operation == CREATE_DIR) {
        if (mkdir(conn->file_attrs.file_path, conn->file_attrs.mode & 0777) < 0) {
          perror("mkdir");
          return -1;
        }
        conn->state = READING_FILE_ATTRS;
        conn->total_read -= attr_size;
        conn->expected_size = sizeof(file_attrs_t);
        conn->file_fd = -1;
      } else if (conn->file_attrs.operation == CREATE_FILE) {
        conn->state = READING_FILE_DATA;
        conn->expected_size = conn->file_attrs.size;
        conn->total_read = 0;
        conn->file_fd = open(conn->file_attrs.file_path, O_CREAT | O_WRONLY | O_TRUNC, conn->file_attrs.mode & 0777);
        
        printf("Updated state %d\n", conn->state);

        if (conn->file_fd < 0) {
          perror("open");
          close(conn->file_fd);
          return -1;
        }

        if (conn->total_read > attr_size) {
          if (write(conn->file_fd, conn->buffer + attr_size, conn->total_read - attr_size) < 0) {
            break;
          }
        } else {
          break;
        }
      } else if (conn->file_attrs.operation == DELETE) {
        conn->state = DELETING_FILE;
        printf("Updated state %d\n", conn->state);
      }
     
    } if (conn->state == READING_FILE_DATA) {
      while (conn->total_read < conn->expected_size) {
        size_t to_read = BUFSIZ;
        if (conn->expected_size - conn->total_read < BUFSIZ) {
          to_read = conn->expected_size - conn->total_read;
        }

        ssize_t n = read(conn->fd, conn->buffer, to_read);
        
        if (n < 0) {
          break;
        }

        if (write(conn->file_fd, conn->buffer, n) < 0) {
          break;
        }

        conn->total_read += n;
      }

      if (conn->total_read == conn->expected_size) {
        conn->state = READING_FILE_ATTRS;
        conn->total_read = 0;
        conn->expected_size = sizeof(file_attrs_t);

        close(conn->file_fd);
        conn->file_fd = -1;
      } else {
        break;
      }
    } else if (conn->state == DELETING_FILE) {
      struct stat st;
      if (stat(conn->file_attrs.file_path, &st) < 0) {
        perror("stat");
        return -1;
      }

      if (S_ISDIR(st.st_mode)) {
        if (remove_dir(conn->file_attrs.file_path)) {
          perror("remove_dir");
          return -1;
        }
      } else {
        if (remove(conn->file_attrs.file_path) < 0) {
          perror("remove");
          return -1;
        }
      }
      conn->state = READING_FILE_ATTRS;
      conn->total_read = 0;
      conn->expected_size = sizeof(file_attrs_t);
      conn->file_fd = -1;
    }
  }

  return 0;
}