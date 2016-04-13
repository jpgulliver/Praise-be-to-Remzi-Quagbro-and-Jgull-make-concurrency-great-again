#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

typedef struct __node
{
    char* data;
    int count;
    struct __node* next;
} Node;

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

int numDowloaders;
int numWorkers;
int queueSize;

// link queue
Node* linkQueue;
pthread_mutex_t linkQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t linkQueueHasSpace = PTHREAD_COND_INITIALIZER;
pthread_cond_t linkQueueHasLinks = PTHREAD_COND_INITIALIZER;

// page queue
Node* pageQueue;
pthread_mutex_t pageQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pageQueueNotEmpty = PTHREAD_COND_INITIALIZER;

//////// stack functions ////////
// returns the new head (NULL)
Node* stack_create()
{
	Node* head = NULL;
	return head; // lol
}

int stack_count(Node* head)
{
	if(head == NULL) {
		return 0;
	}
	return head->count;;
}

// returns the new head
Node* stack_push(Node* head, char* data)
{
	Node* temp;

	if (head == NULL)
	{
		temp = (Node*) malloc(sizeof(Node));	
		assert(temp != NULL);		
		temp->next = NULL;
		temp->data = data;
		temp->count = 1;
		return temp;
	}
	else
	{
		temp = (Node*) malloc(sizeof(Node));
		assert(temp != NULL);
		temp->next = head;
		temp->count = head->count + 1;
		temp->data = data;
		head = temp;
		return head; 
	}
}

// returns the new head
Node* stack_pop(Node* head) {
	char* value;
	Node* temp;
 
	if (head != NULL)
	{
		value = head->data;
		temp = head;
	    
		head = head->next;
		free(temp);
		return head;
	} else {
		return NULL;
	}
}

char* stack_peek(Node* head) {
	if (head != NULL)
	{
		return head->data;
	} else {
		return NULL;
	}

}

void testStack() {
	Node* q1 = stack_create();
	printf("size: %d\n", stack_count(q1));
	
	char* u1 = strdup("url 1");
	q1 = stack_push(q1, u1); 
	printf("size: %d\n", stack_count(q1));
	
	char* u2 = strdup("url 2");
	q1 = stack_push(q1, u2); 
	printf("size: %d\n", stack_count(q1));

	char* u3 = strdup("url 3");
	q1 = stack_push(q1, u3); 
	printf("size: %d\n", stack_count(q1));

	while(q1 != NULL) {
		printf("stack size: %d\n", stack_count(q1));
	
		Node* c = q1;
		while(c != NULL) {
			printf("VAL: %s\n", c->data);
			c = c->next;
		}
		
		printf("\n");
		q1 = stack_pop(q1);
	}
	printf("stack size: %d\n", stack_count(q1));
	
	u1 = strdup("url 2");
	q1 = stack_push(q1, u1); 
	printf("size: %d\n", stack_count(q1));
	
	q1 = stack_pop(q1);
	printf("size: %d\n", stack_count(q1));
}

//////// end stack functions ////////

//////// hash functions ////////
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

struct hashmap* makeHashMap() {
  struct hashmap* a = malloc(sizeof(struct hashmap*));
  a->size = 1024;
  a->buckets = malloc(a->size * sizeof(struct linkedlist*));
  int i = 0;
  for(; i < a->size; i++) { 
    a->buckets[i]->head = NULL;
  }
  return a;
}
  
int checkIfPresent(char* data, struct hashmap* map) {
  pthread_mutex_lock(&(map->lrock));
  unsigned long index = hash((unsigned char*)data) % map->size;
  struct linkedlist* ll = map->buckets[index];
  if (ll->head == NULL) {
    return 0;
  }
  else {
    struct llnode* temp = ll->head;
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
  unsigned long index = hash((unsigned char*)data) % map->size;
  struct linkedlist* ll = map->buckets[index];
  struct llnode* newnode = malloc(sizeof(struct llnode*));
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
//////// end stack functions ////////


// todo:
// parse pages to find links

void addToLinkQueue(char *link) {
	pthread_mutex_lock(&linkQueueLock);
	
	while(stack_count(linkQueue) == queueSize) {
		pthread_cond_wait(&linkQueueHasSpace, &linkQueueLock);
	}
	
	linkQueue = stack_push(linkQueue, link);
	pthread_cond_signal(&linkQueueHasLinks);
	pthread_mutex_unlock(&linkQueueLock);
}

char* getFromLinkQueue() {
	pthread_mutex_lock(&linkQueueLock);
	
	while(stack_count(linkQueue) == 0) {
		pthread_cond_wait(&linkQueueHasLinks, &linkQueueLock);
	}
	
	char *link = stack_peek(linkQueue);
	linkQueue = stack_pop(linkQueue);
		
	pthread_cond_signal(&linkQueueHasSpace);
	pthread_mutex_unlock(&linkQueueLock);

	printf("LINK POPPED: %s\n", link);
	return link;
}

void addPageToQueue(char *page) {
	pthread_mutex_lock(&pageQueueLock);
	
	pageQueue = stack_push(pageQueue, page);
	pthread_cond_signal(&pageQueueNotEmpty);	
	
	pthread_mutex_unlock(&pageQueueLock);

}

// download
char* getPageFromQueue() {
	pthread_mutex_lock(&pageQueueLock);

	while(stack_count(pageQueue) == 0) {
		pthread_cond_wait(&pageQueueNotEmpty, &pageQueueLock);
	}
	
	char *page = stack_peek(pageQueue);
	pageQueue = stack_pop(pageQueue);
		
	pthread_mutex_unlock(&pageQueueLock);

	printf("PAGE POPPED: %s\n", page);	
	return page;
}


void makeContent() {
	/*
	printf("GOT HERE\n");
	
	char* data[5];
	data[0] = strdup("test1");
	data[1] = strdup("this is also a test");
	data[2] = strdup("I am still testing");
	data[3] = strdup("linix pls");
	data[4] = strdup("please");
	
	int i;
	for(i = 0; i < 5; i++) {
		printf("%d ADDING DATA: %s\n", i, data[i]);
		addToLinkQueue(data[i]);
	}
	*/
	
	printf("GOT HERE\n");
	char* data[5];
	data[0] = strdup("page 1");
	data[1] = strdup("page 2");
	data[2] = strdup("page 3");
	data[3] = strdup("page page page page 4");
	data[4] = strdup("testtesttest");
	
	int i;
	for(i = 0; i < 5; i++) {
		printf("%d ADDING PAGES: %s\n", i, data[i]);
		addPageToQueue(data[i]);
	}
	
}

void consumeContent() {
	/*
	int i;
	for(i = 0; i < 5; i++) {
		getFromLinkQueue();	
	}
	*/
	
	int i;
	for(i = 0; i < 5; i++) {
		getPageFromQueue();	
	}
}



int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {
	  
	numDowloaders = download_workers;
	numWorkers = parse_workers;
	queueSize = queue_size;  
	
	pthread_t p1;
	pthread_t p2;
	
	pthread_create(&p2, NULL, (void *) &consumeContent, NULL);
	pthread_create(&p1, NULL, (void *) &makeContent, NULL);
	
	pthread_join(p1, NULL);
    	pthread_join(p2, NULL);

	printf("TESST\n");
	//testStack();

	return 0;
	//return -1;
}
