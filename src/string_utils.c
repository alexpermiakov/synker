#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include "string_utils.h"

char *get_postfix(char *full_path, char *prefix) {
  return full_path + strlen(prefix);
}

char *get_src_path(char *base_path, char *filename) {
  char *new_path = malloc(PATH_MAX);
  
  if (base_path == NULL) {
    sprintf(new_path, "%s", filename);
  } else { 
    sprintf(new_path, "%s/%s", base_path, filename);
  }

  return new_path;
}

char *get_dst_path(char *src, char *src_prefix, char *dst_prefix) {
  char *postfix = get_postfix(src, src_prefix);
  char *dst_full_path = malloc(PATH_MAX);
  snprintf(dst_full_path, PATH_MAX, "%s%s", dst_prefix, postfix);

  return dst_full_path;
}