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

void copy_dir (int client_fd, char *src_file_path, char *dst_file_path) {
  if (!is_dir_exists(dst_file_path) && mkdir(dst_file_path, 0777) == -1) {
    perror("mkdir");
    exit(1);
  }

  DIR *dir = opendir(src_file_path);

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
    char sub_dst_file_path[PATH_MAX];

    snprintf(src_path, sizeof(src_path), "%s/%s", src_file_path, entry->d_name);
    snprintf(sub_dst_file_path, sizeof(sub_dst_file_path), "%s/%s", dst_file_path, entry->d_name);

    if (entry->d_type == DT_DIR) {
      copy_dir(client_fd, src_path, sub_dst_file_path);
    } else {
      copy_file(client_fd, src_path, sub_dst_file_path);
    }
  }

  closedir(dir);
}