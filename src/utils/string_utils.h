#ifndef STRING_UTILS_H
#define STRING_UTILS_H

char *get_postfix(char *full_path, char *prefix);
char *get_src_path(char *base_path, char *filename);
char *get_dst_path(char *src, char *src_prefix, char *dst_prefix);

#endif