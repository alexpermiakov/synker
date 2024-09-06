#ifndef EPOLL_HELPER_H
#define EPOLL_HELPER_H

#include <sys/epoll.h>

#define MAX_EVENTS 100 // more than 1000 is good for high-performance monitoring

int epoll_init();
void epoll_add_fd(int epoll_fd, int ifd);
int epoll_wait_for_events(int epoll_fd, struct epoll_event *events, int max_events);

#endif