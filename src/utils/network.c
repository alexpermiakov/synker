#include <stdio.h>
#include <linux/limits.h>
#include <string.h>
#include <stdlib.h>

#include "network.h"

void extract_connection_info(char *full_path, char *domain_name, int *port, char *file_path) {
  strcpy(domain_name, strtok(full_path, ":"));
  *port = atoi(strtok(NULL, "/"));
  sprintf(file_path, "/%s", strtok(NULL, ""));
}