#include <stdio.h>
#include <linux/limits.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "network.h"
#include "file_utils.h"

void extract_connection_info(char *full_path, char *server_ip, int *port, char *file_path) {
  strcpy(server_ip, strtok(full_path, ":"));
  *port = atoi(strtok(NULL, "/"));
  sprintf(file_path, "/%s", strtok(NULL, ""));
}

int connect_to_server(char *server_ip, int port) {
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in server_addr;

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET; 
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(server_ip);

  while(1) {
    printf("Connecting to %s:%d...\n", server_ip, port);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0) {
      break;
    }

    sleep(3);
  }
  printf("Connected to %s:%d\n", server_ip, port);

  return sock_fd;
}