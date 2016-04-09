#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

struct llnode {
  struct llnode* next;
  char* data;
};

struct linkedlist {
  struct llnode* head;
};

struct hashmap {
  int size;
  struct linkedlist** buckets;
  pthread_mutex_t lrock;
};

//http://www.cse.yorku.ca/~oz/hash.html
//djb2 by Dan Bernstein
unsigned long
hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void makeHashMap(struct hashmap* a) {
  a->size = 1024;
  a->buckets = malloc(a->size * sizeof(struct linkedlist*));
  int i = 0;
  for(; i < a->size; i++) { 
    struct linkedlist* newll = malloc(sizeof(struct linkedlist*));
    newll->head = NULL;
    a->buckets[i] = newll;
  }
}
  
int checkIfPresent(char* data, struct hashmap* map) {
  pthread_mutex_lock(&(map->lrock));
  int rflag = 0;
  unsigned long index = hash((unsigned char*)data) % map->size;
  struct linkedlist* ll = map->buckets[index];
  if (ll->head != NULL) {
    struct llnode* temp = ll->head;
    while (temp->next != NULL) {
      if (strcmp(temp->data, data) == 0)
	rflag = 1;
      temp = temp->next;
    }
    if (strcmp(temp->data, data) == 0) {
      rflag = 1;
    }
  }  
  pthread_mutex_unlock(&(map->lrock));
  return rflag;
}

int put(char* data, struct hashmap* map) {
  pthread_mutex_lock(&(map->lrock));
  unsigned long index = hash((unsigned char*)data) % map->size;
  struct linkedlist* ll = map->buckets[index];
  struct llnode* newnode = malloc(sizeof(struct llnode*));
  newnode->next = NULL;
  newnode->data = data;
  if (ll->head == NULL) {
    ll->head = newnode;
  }
  else {
    struct llnode* temp = ll->head;
    while (temp->next != NULL) {
      temp = temp->next;
    }
    temp->next = newnode;
  }
  pthread_mutex_unlock(&(map->lrock));
  return 1;
}

int main() {
  
  //testing
  struct hashmap map;
  makeHashMap(&map);
  put("helloworld\n", &map);
  printf("%d\n", checkIfPresent("helloworld\n", &map));
  printf("%d\n", checkIfPresent("hello\n", &map));
  put("helloworld\n", &map);  
  printf("%d\n", checkIfPresent("helloworld\n", &map));
  printf("%d\n", checkIfPresent("hello\n", &map));

  return 1;
}

