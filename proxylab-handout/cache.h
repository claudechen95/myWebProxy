/*
 * header file for cache.c
 */

#include "csapp.h"
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400


typedef struct cache_entry{
  char uri[MAXLINE];
  char content[MAX_OBJECT_SIZE];
  size_t LRU;
  int valid;
  int size;
} cache_entry;

void init_cache(void);
cache_entry* find(char* uri);
void insert(char *uri,char *content);
void clean_up(void);

