#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

typedef struct __node
{
    char* url;
    int count;
    struct __node* next;
} Node;

int numDowloaders;
int numWorkers;
int queueSize;

// link queue
Node* linkQueue;
pthread_mutex_t linkQueueLock;

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
Node* stack_push(Node* head, char* url)
{
	Node* temp;

	if (head == NULL)
	{
		temp = (Node*) malloc(sizeof(Node));	
		assert(temp != NULL);		
		temp->next = NULL;
		temp->url = url;
		temp->count = 1;
		return temp;
	}
	else
	{
		temp = (Node*) malloc(sizeof(Node));
		assert(temp != NULL);
		temp->next = head;
		temp->count = head->count + 1;
		temp->url = url;
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
		value = head->url;
		temp = head;
	    
		head = head->next;
		
		if(temp->url != NULL) {
			free(temp->url);
		}
		free(temp);
		return head;
	} else {
		return NULL;
	}
}

char* stack_peek(Node* head) {
	if (head != NULL)
	{
		return head->url;
	} else {
		return NULL;
	}

}

//////// end stack functions ////////


void addToLinkQueue(char *link) {
	pthread_mutex_lock(&linkQueueLock);
	
	
	pthread_mutex_unlock(&linkQueueLock);
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
			printf("VAL: %s\n", c->url);
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

int crawl(char *start_url,
	  int download_workers,
	  int parse_workers,
	  int queue_size,
	  char * (*_fetch_fn)(char *url),
	  void (*_edge_fn)(char *from, char *to)) {
	  
	numDowloaders = download_workers;
	numWorkers = parse_workers;
	queueSize = queue_size;  

	testStack();

	return 0;
	//return -1;
}