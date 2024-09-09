#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>

#include "epoll.h"

int epoll_init() {
  int epoll_fd = epoll_create1(0);

  if (epoll_fd < 0) {
    perror("epoll");
    exit(1);
  }

  return epoll_fd;
}

void epoll_add_fd(int epoll_fd, int ifd) {
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = ifd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ifd, &event) < 0) {
    perror("epoll_ctl");
    exit(1);
  }
}

int epoll_wait_for_events(int epoll_fd, struct epoll_event *events, int max_events) {
  int num_events = epoll_wait(epoll_fd, events, max_events, -1);

  if (num_events < 0) {
    perror("epoll_wait");
    exit(1);
  }

  return num_events;
}