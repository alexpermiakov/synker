#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <stdio.h>
#include "utils/file_utils.h"

typedef enum {
  READING_FILE_ATTRS,
  READING_FILE_DATA,
  DELETING_FILE
} connection_state_t;

typedef struct {
  int fd;                     // client socket file descriptor
  connection_state_t state;   // current state of the connection
  char buffer[BUFSIZ];        // buffer for reading/writing data
  size_t buffer_size;         // size of the buffer
  size_t current_read;          // total bytes read so far
  size_t total_written;       // total bytes written so far
  size_t expected_size;       // expected total size of the data to be read
  file_attrs_t file_attrs;    // file attributes
  int file_fd;                // file descriptor for the file being written
} connection_t;

int handle_client(connection_t *conn);

#endif