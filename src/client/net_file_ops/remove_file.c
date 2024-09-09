#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "remove_file.h"

void remove_file(char *path) {
  if (unlink(path) == -1) {
    perror("unlink");
    exit(1);
  }
}