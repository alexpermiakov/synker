#include <stdio.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "utils/file_utils.h"
#include "client_handler.h"

int handle_reading_file_state(connection_t *conn) {
  printf("Reading file data\n");

  while (conn->current_read < conn->expected_size) {
    size_t to_read = BUFSIZ;
    if (conn->expected_size - conn->current_read < BUFSIZ) {
      to_read = conn->expected_size - conn->current_read;
    }

    ssize_t n = read(conn->fd, conn->buffer, to_read);
    
    if (n < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        return 0;
      }
      return -1;
    }

    if (write(conn->file_fd, conn->buffer, n) < 0) {
      return -1;
    }

    conn->current_read += n;
  }

  if (conn->current_read == conn->expected_size) {
    conn->state = READING_FILE_ATTRS;
    conn->current_read = 0;
    conn->expected_size = sizeof(file_attrs_t);

    close(conn->file_fd);
  }

  return 0;
}

int handle_deleting_state (connection_t *conn) {
  printf("Deleting file\n");

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
  conn->current_read = 0;
  conn->expected_size = sizeof(file_attrs_t);

  return 0;
}

int handle_file_attr_state (connection_t *conn) {
  printf("Reading file attrs\n");
  size_t attr_size = sizeof(file_attrs_t);
  ssize_t n = read(conn->fd, conn->buffer + conn->current_read, attr_size - conn->current_read);
  
  if (n < 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      return 0;
    }
    return -1;
  }

  conn->current_read += n;

  if (conn->current_read < attr_size) {
    return 0;
  }

  deserialize_file_attrs(&conn->file_attrs, conn->buffer);
  conn->current_read = 0;

  printf("File path: %s\n", conn->file_attrs.file_path);
  printf("Operation: %d\n", conn->file_attrs.operation);

  if (conn->file_attrs.operation == CREATE_DIR) {
    if (is_dir_exists(conn->file_attrs.file_path)) {
      return 0;
    }

    if (mkdir(conn->file_attrs.file_path, conn->file_attrs.mode & 0777) < 0) {
      perror("mkdir");
      return -1;
    }
  }
  
  if (conn->file_attrs.operation == CREATE_FILE) {
    conn->file_fd = open(conn->file_attrs.file_path, O_CREAT | O_WRONLY | O_TRUNC, conn->file_attrs.mode & 0777);

    if (conn->file_fd < 0) {
      perror("open");
      return -1;
    }

    conn->state = READING_FILE_DATA;
    conn->expected_size = conn->file_attrs.size;
  }
  
  if (conn->file_attrs.operation == DELETE) {
    conn->state = DELETING_FILE;
  }

  return 0;
}

int handle_client(connection_t *conn) {
  while (1) {
    switch(conn->state) {
      case READING_FILE_ATTRS:
        return handle_file_attr_state(conn);
      case READING_FILE_DATA:
        return handle_reading_file_state(conn);
      case DELETING_FILE:
        return handle_deleting_state(conn);
    }
  }

  return 0;
}