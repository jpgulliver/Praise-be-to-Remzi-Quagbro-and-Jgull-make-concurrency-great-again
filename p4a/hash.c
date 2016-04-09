#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

struct llnode {
  struct llnode* next = NULL;
  char* data = NULL;
};

struct linkedlist {
  struct llnode* head = NULL;
};

struct hashmap {
  int size = 1024;
  struct linkedlist* buckets;
  pthread_mutex_t lrock;
};

//http://www.cse.yorku.ca/~oz/hash.html
//djb2 by Dan Bernstein
unsigned long
hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

struct hashmap makeHashMap() {
  struct hashmap a;
  a->buckets = malloc(a->size * sizeof(linkedlist*));
  return a;
}
  
int checkIfPresent(char* data, struct hashmap* map) {
  pthread_mutex_lock(&(map->lrock));
  unsigned long index = hash(data) % map->size;
  ll = map->buckets[index];
  if (ll->head == NULL) {
    return 0;
  }
  else {
    struct node* temp = ll->head;
    while (temp->next != NULL) {
      if (strcmp(temp->data, data) == 0)
	return 1;
      temp = temp->next;
    }
    if (strcmp(temp->data, data) == 0)
      return 1;
    return 0;
  }  
  pthread_mutex_unlock(&(map->lrock));
}

int put(char* data, struct hashmap* map) {
  pthread_mutex_lock(&(map->lrock));
  unsigned long index = hash(page) % map->size;
  ll = map->buckets[index];
  struct llnode* newnode = malloc(sizeof(llnode*));
  newnode->data = data;
  if (ll->head == NULL) {
    ll->head = newnode;
  }
  else {
    struct node* temp = ll->head;
    while (temp->next != NULL) {
      temp = temp->next;
    }
    temp->next = newnode;
  }
  pthread_mutex_unlock(&(map->lrock));
}






