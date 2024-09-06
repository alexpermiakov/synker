#include <stdio.h>
#include <linux/limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "file_operations.h"

bool is_dir_exists(char *path) {
  struct stat info;
  
  if (lstat(path, &info)) {
    return 0;
  }

  return info.st_mode & S_IFDIR;
}

void copy_file (char *src, char *dst) {
  int src_fd = open(src, O_RDONLY);
  
  if (src_fd == -1) {
    perror("open");
    exit(1);
  }

  int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 644);

  if (dst_fd == -1) {
    perror("open");
    exit(1);
  }

  ssize_t nread;
  char buf[BUFSIZ];

  while ((nread = read(src_fd, buf, sizeof(buf))) > 0) {
    if (write(dst_fd, buf, nread) != nread) {
      perror("write");
      close(src_fd);
      close(dst_fd);
      exit(1);
    }
  }

  if (nread == -1) {
    perror("read");
    close(src_fd);
    close(dst_fd);
    exit(1);
  }

  close(src_fd);
  close(dst_fd);
}

void copy_dir (char *src, char *dst) {
  if (!is_dir_exists(dst) && mkdir(dst, 0777) == -1) {
    perror("mkdir");
    exit(1);
  }

  DIR *dir = opendir(src);

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

    snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
    snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

    if (entry->d_type == DT_DIR) {
      copy_dir(src_path, dst_path);
    } else {
      copy_file(src_path, dst_path);
    }
  }

  closedir(dir);
}

void remove_file(char *path) {
  if (unlink(path) == -1) {
    perror("unlink");
    exit(1);
  }
}

void remove_dir(char *path) {
  DIR *dir = opendir(path);

  if (dir == NULL) {
    perror("opendir");
    exit(1);
  }

  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

    if (entry->d_type == DT_DIR) {
      remove_dir(full_path);
    } else {
      remove_file(full_path);
    }
  }

  closedir(dir);

  if (rmdir(path) == -1) {
    perror("rmdir");
    exit(1);
  }
}