#ifndef NETWORK_H
#define NETWORK_H

int connect_to_server(char *server_url);
void add_to_epoll(int *epoll_fd, int fd);
void extract_connection_info(char *full_path, char *domain, int *port, char *file_path);

#endif