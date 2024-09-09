#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "utils/file_utils.h"
#include "copy_dir.h"
#include "copy_file.h"
#include "remove_dir.h"
#include "remove_file.h"

void copy_dir (char *src_full_path, char *dst) {
  if (!is_dir_exists(dst) && mkdir(dst, 0777) == -1) {
    perror("mkdir");
    exit(1);
  }

  DIR *dir = opendir(src_full_path);

  if (dir == NULL) {
    perror("opendir");
    exit(1);
  }

  struct dirent *entry;

  while((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char src_path[PATH_MAX];
    char dst_path[PATH_MAX];

    snprintf(src_path, sizeof(src_path), "%s/%s", src_full_path, entry->d_name);
    snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

    if (entry->d_type == DT_DIR) {
      copy_dir(src_path, dst_path);
    } else {
      copy_file(src_path, dst_path);
    }
  }

  closedir(dir);
}