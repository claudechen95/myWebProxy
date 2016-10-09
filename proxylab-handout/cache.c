/*
 * This is a cache that is implemented as an array of length 10.
 * basic functionality includes:
 * find if theres a cache hit.
 * insert a cache block
 * free up the allocated cache. 
*/

#include "csapp.h"
#include <limits.h>
#include "cache.h"
#define MAXLINENUMBER (MAX_CACHE_SIZE/MAX_OBJECT_SIZE)




cache_entry **C;
int Time = 0;

/* initialize cache */
void init_cache(){

  C = (cache_entry**)Malloc(MAXLINENUMBER*sizeof(cache_entry*));
  size_t i;
  for ( i = 0; i < MAXLINENUMBER; ++i)
  {
    C[i] = (cache_entry*)Malloc(sizeof(cache_entry));
    C[i]->LRU = 0;  
    C[i]->valid = 0;
  }
}


/*find if a given uri is cached*/
cache_entry* find(char* uri){
  cache_entry* p;
  size_t i;
  for( i = 0;i<10;i++){
    p = C[i];
    if (p->valid){
        if(strcmp(p->uri,uri)==0){
          p->LRU = Time;
          return p;
        }
      }
  }
  return NULL;
}


/*insert a cache block into cache obeying the LRU policy,LRU is evicted*/
void insert(char *uri,char *content){
  int i;
  cache_entry* p;
  int min = INT_MAX;
  int minIndex;
  for (i = 0; i < 10;i++){
    p = C[i];
    if(p->valid == 0){
      strcpy(p->uri,uri);
      strcpy(p->content,content);
      p->size = strlen(content);
      p->valid = 1;
      p->LRU = Time;
      return;
    }
    if(p->LRU <min){
      min = p->LRU;
      minIndex = i;
    }
  }
  p = C[minIndex];
  strcpy(p->uri,uri);
  strcpy(p->content,content);
  p->size = strlen(content);
  p->LRU = Time;
}

/*free up the cache*/
void clean_up(){
  int i;
  for (i = 0; i < 10; ++i)
  {
    Free(C[i]);
  }
  Free(C);
}

