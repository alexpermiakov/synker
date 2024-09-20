#ifndef epoll_H
#define epoll_H

#include <sys/epoll.h>

#define MAX_EVENTS 1000

int epoll_init();
void epoll_add_fd(int epoll_fd, int inotify_fd);
int epoll_wait_for_events(int epoll_fd, struct epoll_event *events, int max_events);

#endif