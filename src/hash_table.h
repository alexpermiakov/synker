#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <pthread.h>

#define HASH_TABLE_SIZE 10000

typedef struct HashNode {
  char *key;
  void *val;
  struct HashNode *next;
} HashNode;

typedef struct {
  pthread_mutex_t locks[HASH_TABLE_SIZE];
  HashNode *nodes[HASH_TABLE_SIZE];
} HashTable;

typedef void (*CleanupHandler)(char *key, void *val);
typedef void (*Iterator)(HashNode *, char*);
typedef void (*PrintHandler)(char *key, void *val);

void hash_table_init(HashTable *);
void hash_table_iterate(HashTable *, Iterator, char*);
void hash_table_set(HashTable *, char *, void *);
void *hash_table_get(HashTable *, char *);
void remove_hash_node(HashTable *, char *, CleanupHandler);
void hash_table_dispose(HashTable *, CleanupHandler);
void hash_table_print(HashTable *, PrintHandler);

unsigned int hash(char *);

#endif