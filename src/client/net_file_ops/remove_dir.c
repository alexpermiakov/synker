#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "remove_dir.h"
#include "delete_file.h"

// void remove_dir(char *path) {
//   DIR *dir = opendir(path);

//   if (dir == NULL) {
//     perror("opendir");
//     exit(1);
//   }

//   struct dirent *entry;

//   while ((entry = readdir(dir)) != NULL) {
//     if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
//       continue;
//     }

//     char full_path[PATH_MAX];
//     snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

//     if (entry->d_type == DT_DIR) {
//       remove_dir(full_path);
//     } else {
//       delete_file(full_path);
//     }
//   }

//   closedir(dir);

//   if (rmdir(path) == -1) {
//     perror("rmdir");
//     exit(1);
//   }
// }