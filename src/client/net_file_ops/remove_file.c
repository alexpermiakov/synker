#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <linux/limits.h>

#include "remove_file.h"

void remove_file(int client_fd, char *dst_file_path) {
  if(write(client_fd, dst_file_path, strlen(dst_file_path) + 1) == -1) {
    perror("write");
    close(client_fd);
    exit(1);
  }  
}