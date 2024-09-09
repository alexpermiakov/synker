#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>

#include "hash_table.h"

unsigned int hash(char *key) {
  size_t hash = 0;
  
  while(*key != '\0') {
    // this is same as: hash = hash * 31 + *key;
    hash = (hash << 5) - hash + *key;
    key++;
  }

  return hash % HASH_TABLE_SIZE;
}

void hash_table_init(HashTable *table) {
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    pthread_mutex_init(&table->locks[i], NULL);
  }

  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    table->nodes[i] = NULL;
  }
}

void hash_table_iterate(HashTable *table, Iterator iterator, char *args) {
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    pthread_mutex_lock(&table->locks[i]);
    HashNode *node = table->nodes[i];

    while (node != NULL) {
      iterator(node, args);
      node = node->next;
    }

    pthread_mutex_unlock(&table->locks[i]);
  }
}

void hash_table_print(HashTable *table, PrintHandler print_handler) {  
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    pthread_mutex_lock(&table->locks[i]);
    HashNode *node = table->nodes[i];

    while (node != NULL) {
      print_handler(node->key, node->val);
      node = node->next;
    }

    pthread_mutex_unlock(&table->locks[i]);
  }
}

void hash_table_set(HashTable *table, char *key, void *val) {
  unsigned int index = hash(key);
  pthread_mutex_lock(&table->locks[index]);
  HashNode *node = table->nodes[index];

  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      break;
    }
    node = node->next;
  }

  if (node != NULL) {
    node->val = val;
  } else {
    HashNode *newNode = malloc(sizeof(HashNode));
    if (newNode == NULL) {
      pthread_mutex_unlock(&table->locks[index]);
      perror("malloc");
      exit(1);
    }

    newNode->key = key;
    newNode->val = val;
    newNode->next = node;
    table->nodes[index] = newNode;
  }

  pthread_mutex_unlock(&table->locks[index]);
}

void *hash_table_get(HashTable *table, char *key) {
  unsigned int index = hash(key);
  
  pthread_mutex_lock(&table->locks[index]);
  HashNode *node = table->nodes[index];
  
  if (node == NULL) {
    return NULL;
  }

  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      pthread_mutex_unlock(&table->locks[index]);
      return node->val;
    }
    node = node->next;
  }

  pthread_mutex_unlock(&table->locks[index]);

  return NULL;
}

typedef void (*CleanupHandler)(char *key, void *val);

void remove_hash_node(HashTable *table, char *key, CleanupHandler cleanup_handler) {
  unsigned int index = hash(key);

  pthread_mutex_lock(&table->locks[index]);
  HashNode *node = table->nodes[index];
  HashNode *prev = NULL;

  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      if (prev == NULL) {
        table->nodes[index] = node->next;
      } else {
        prev->next = node->next;
      }
      if (cleanup_handler != NULL) {
        cleanup_handler(node->key, node->val);
      }
      free(node);
      pthread_mutex_unlock(&table->locks[index]);
      return;
    }
    prev = node;
    node = node->next;
  }

  pthread_mutex_unlock(&table->locks[index]);
}

void hash_table_dispose(HashTable *table, CleanupHandler cleanup_handler) {
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    pthread_mutex_lock(&table->locks[i]);
    HashNode *node = table->nodes[i];
    
    while (node != NULL) {
      HashNode *next = node->next;
      if (cleanup_handler != NULL) {
        cleanup_handler(node->key, node->val);
      }
      free(node);
      node = next;
    }
    pthread_mutex_unlock(&table->locks[i]);
  }

  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    pthread_mutex_destroy(&table->locks[i]);
  }
}