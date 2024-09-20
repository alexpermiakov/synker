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
      ssize_t n = read(conn->fd, conn->buffer + conn->total_read, sizeof(conn->buffer) - conn->total_read);
      
      if (n <= 0) {
        if (n < 0) {
          perror("read");
        }
        break;
      }
      
      conn->total_read += n;
      size_t offset = 0;
      
      // Process all complete file_attrs_t structures in the buffer
      while (conn->total_read - offset >= attr_size) {
        deserialize_file_attrs(&conn->file_attrs, conn->buffer + offset);

        if (conn->file_attrs.operation == CREATE_DIR) {
          if (mkdir(conn->file_attrs.file_path, conn->file_attrs.mode & 0777) < 0) {
            perror("mkdir"); return -1; 
          }
        } else if (conn->file_attrs.operation == CREATE_FILE) {
          conn->state = READING_FILE_DATA;
          conn->expected_size = conn->file_attrs.size;
          conn->file_fd = open(conn->file_attrs.file_path, O_CREAT | O_WRONLY | O_TRUNC, conn->file_attrs.mode & 0777);
          
          if (conn->file_fd < 0) {
            perror("open");
            return -1;
          }
          
          // Check if there's file data already in the buffer
          size_t data_in_buffer = conn->total_read - offset - attr_size;
          if (data_in_buffer > 0) {
            size_t to_write = data_in_buffer < conn->expected_size ? data_in_buffer : conn->expected_size;
            
            if (write(conn->file_fd, conn->buffer + offset + attr_size, to_write) < 0) {
              perror("write");
              return -1;
            }
            
            conn->expected_size -= to_write;
            offset += attr_size + to_write;
            
            if (conn->expected_size == 0) {
              close(conn->file_fd);
              conn->file_fd = -1;
              conn->state = READING_FILE_ATTRS;
            } else { 
              // Need to read more file data
              conn->total_read -= offset;
              memmove(conn->buffer, conn->buffer + offset, conn->total_read);
              break;
            }
          } else {
            // No file data in buffer, adjust and break to read more
            offset += attr_size;
            conn->total_read -= offset;
            memmove(conn->buffer, conn->buffer + offset, conn->total_read);
            break;
          }
        } else if (conn->file_attrs.operation == DELETE) {
            struct stat st;
            if (stat(conn->file_attrs.file_path, &st) < 0) {
              perror("stat");
              return -1;
            }
            if (S_ISDIR(st.st_mode)) {
              if (remove_dir(conn->file_attrs.file_path)) {
                perror("remove_dir"); return -1;
              } 
            } else {
              if (remove(conn->file_attrs.file_path) < 0) {
                perror("remove");
                return -1;
              }
            }
            
            offset += attr_size;
        } else {
          fprintf(stderr, "Unknown operation\n"); return -1;
        }
      }
      
      // Adjust the buffer for any leftover data
        
      conn->total_read -= offset;
      
      if (conn->total_read > 0) {
        memmove(conn->buffer, conn->buffer + offset, conn->total_read);
      } else {
        conn->total_read = 0;
      }
    } else if (conn->state == READING_FILE_DATA) {
      // Read file data
      while (conn->expected_size > 0) {
        ssize_t n = read(conn->fd, conn->buffer, sizeof(conn->buffer));
        if (n <= 0) {
          if (n < 0) perror("read");
          break;
        }
        
        size_t to_write = (size_t)n < conn->expected_size ? (size_t)n : conn->expected_size;
        
        if (write(conn->file_fd, conn->buffer, to_write) < 0) {
          perror("write");
          return -1;
        }
        
        conn->expected_size -= to_write;
        
        if (conn->expected_size == 0) {
          close(conn->file_fd);
          conn->file_fd = -1;
          conn->state = READING_FILE_ATTRS;
          
          if ((size_t)n > to_write) {
            conn->total_read = n - to_write;
            memmove(conn->buffer, conn->buffer + to_write, conn->total_read);
          } else { 
            conn->total_read = 0;
          }
          
          break;
        }
      }
    }
  }

  return 0;
}