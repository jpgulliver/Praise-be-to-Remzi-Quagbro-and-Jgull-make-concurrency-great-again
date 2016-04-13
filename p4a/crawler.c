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
int numParsers;
int queueSize;
struct hashmap map;
char * (*fetch_fn)(char *url);
int done;

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

//////// end hash functions ////////

void addToLinkQueue(char *link) {
	pthread_mutex_lock(&linkQueueLock);
	
	while(stack_count(linkQueue) == queueSize) {
		pthread_cond_wait(&linkQueueHasSpace, &linkQueueLock);
	}
	
	linkQueue = stack_push(linkQueue, link);
	
	printf("LINK PUSHED: %s\n", link);
	
	pthread_cond_signal(&linkQueueHasLinks);
	pthread_mutex_unlock(&linkQueueLock);
}

char* getFromLinkQueue() {
	pthread_mutex_lock(&linkQueueLock);
	
	while(stack_count(linkQueue) == 0) {
		pthread_cond_wait(&linkQueueHasLinks, &linkQueueLock);
		
		if(done) {
			pthread_mutex_unlock(&linkQueueLock);
			return NULL;
		}
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

char* getPageFromQueue() {
	pthread_mutex_lock(&pageQueueLock);

	while(stack_count(pageQueue) == 0) {
		pthread_cond_wait(&pageQueueNotEmpty, &pageQueueLock);
		
		if(done) {
			pthread_mutex_unlock(&pageQueueLock);
			return NULL;
		}
	}
	
	char *page = stack_peek(pageQueue);
	pageQueue = stack_pop(pageQueue);
		
	pthread_mutex_unlock(&pageQueueLock);

	//printf("PAGE POPPED: [[[[[[[[[\n%s\n]]]]]]]]]\n", page);	
	return page;
}

Node* parsePage(char* page) {
	int i, j;
	i = 0;
	
	Node* listOfPages = NULL;
	
	while(page[i] != '\0') {
		if(page[i + 0] != 'l') { 
			i++; 
			continue;
		}
		if(page[i + 1] != 'i') { 
			i++; 
			continue;
		}
		if(page[i + 2] != 'n') { 
			i++; 
			continue;
		}
		if(page[i + 3] != 'k') { 
			i++; 
			continue;
		}
		if(page[i + 4] != ':') { 
			i++; 
			continue;
		}
		i = i + 5;
		j = i;
		
		while(page[j] != ' ' && page[j] != '\t' && page[j] != '\0' && page[j] != '\n') {
			j++;
		}
		
		char* newLink = malloc(sizeof(char) * (j - i + 1));
		strncpy(newLink, page + i, j - i);
		listOfPages = stack_push(listOfPages, newLink);
		i++;
	}
	
	return listOfPages;	
}

void download() {
	while(1) {
		char* link = getFromLinkQueue(); 	
		if(done) {
			return;
		}
		
		char *page = (*fetch_fn)(link);
		addPageToQueue(page);
		//free(link);
	}
}

void parse() {
	while(1) {
		char* page = getPageFromQueue();
		if(done) {
			return;
		}
		
		Node* parsed = parsePage(page);
	
		while(parsed != NULL) {
			char* data = stack_peek(parsed);
			parsed = stack_pop(parsed);
			
			if(!checkIfPresent(data, &map)) {
				addToLinkQueue(data);
				put(data, &map);
			
			}
		}
	
		free(page);
	}
}

void test() {
	char *page = (*fetch_fn)("pageb");
	Node* parsed = parsePage(page);

	while(parsed != NULL) {
		char* data = stack_peek(parsed);
		parsed = stack_pop(parsed);
		printf("%s\n", data);

	}
}

int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {
	  	  
	int i;  
	numDowloaders = download_workers;
	numParsers = parse_workers;
	queueSize = queue_size;  
	fetch_fn = _fetch_fn;
	done = 0;
	
	//test();
	//return 0;
	
	makeHashMap(&map);
	
	// parse the first page
	char *startPage = (*_fetch_fn)(start_url);
	pageQueue = stack_push(pageQueue, startPage);
	
	// start threads for the workers
	pthread_t* downloaders = malloc(sizeof(pthread_t) * numDowloaders); 
	for(i = 0; i < numDowloaders; i++) {
		if (pthread_create(&downloaders[i], NULL, (void *) &download, NULL) != 0)
		{
			return -1;
		}
	}
	
	pthread_t* parsers = malloc(sizeof(pthread_t) * numParsers); 
	for(i = 0; i < numParsers; i++) {
		if (pthread_create(&parsers[i], NULL, (void *) &parse, NULL) != 0)
		{
			return -1;
		}
	}
	
	// signal that we are all done
	while(1) {	
		if(stack_count(linkQueue) == 0 && stack_count(pageQueue) == 0) {
			done = 1;
			pthread_cond_signal(&linkQueueHasLinks);
			pthread_cond_signal(&linkQueueHasSpace);
			pthread_cond_signal(&pageQueueNotEmpty);	
			break;
		}
		
		sleep(1);
	
	}
	
	// wait for the web crawl to terminate
	for(i = 0; i < numDowloaders; i++) {
		pthread_join(downloaders[i], NULL);
	}
	free(downloaders);
	
	for(i = 0; i < numParsers; i++) {
		pthread_join(parsers[i], NULL);
	}
	free(parsers);
		
	return 0;
}
