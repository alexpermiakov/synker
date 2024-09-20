#include <stdio.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#include "utils/file_utils.h"
#include "client_handler.h"

int handle_client(connection_t *conn) {
  while (1) {
    if (conn->state == READING_FILE_ATTRS) {
      size_t attr_size = sizeof(file_attrs_t);
      ssize_t n = read(conn->fd, conn->buffer + conn->total_read, attr_size - conn->total_read);
      
      if (n < 0) {
        break;
      }

      conn->total_read += n;

      if (conn->total_read < attr_size) {
        break;
      }

      deserialize_file_attrs(&conn->file_attrs, conn->buffer);
      size_t excess_bytes = conn->total_read - attr_size;

      if (conn->file_attrs.operation == CREATE_DIR) {
        if (mkdir(conn->file_attrs.file_path, conn->file_attrs.mode & 0777) < 0) {
          perror("mkdir");
          return -1;
        }
        conn->state = READING_FILE_ATTRS;
        conn->total_read = 0;
        conn->expected_size = sizeof(file_attrs_t);
        conn->file_fd = -1;
      } else if (conn->file_attrs.operation == CREATE_FILE) {
        conn->state = READING_FILE_DATA;
        conn->expected_size = conn->file_attrs.size;
        conn->total_read = excess_bytes;
        conn->file_fd = open(conn->file_attrs.file_path, O_CREAT | O_WRONLY | O_TRUNC, conn->file_attrs.mode & 0777);

        if (conn->file_fd < 0) {
          perror("open");
          close(conn->file_fd);
          return -1;
        }

        if (excess_bytes > 0) {
          if (write(conn->file_fd, conn->buffer + attr_size, excess_bytes) < 0) {
            break;
          }
        } else {
          break;
        }
      } else if (conn->file_attrs.operation == DELETE) {
        conn->state = DELETING_FILE;
      }

      if (excess_bytes == 0) {
        conn->total_read = 0;
      }

      if (excess_bytes > 0) {
        memmove(conn->buffer, conn->buffer + attr_size, excess_bytes);
        conn->total_read = excess_bytes;
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