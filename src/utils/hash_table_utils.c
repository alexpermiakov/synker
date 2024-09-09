#include <stdio.h>
#include <stdlib.h>

#include "hash_table_utils.h"

void deallocate_hash_key_values(char *key, void *val) {
  free(key);
  free(val);
}

void print_string(char *key, void *val) {
  printf("key: %s, value: %s\n", key, (char *) val);
}

void print_int(char *key, void *val) {
  printf("key: %s, value: %d\n", key, *(int *) val);
}