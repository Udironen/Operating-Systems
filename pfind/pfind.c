// A C program to demonstrate linked list based implementation of queue 
#include <stdio.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>
#include <dirent.h>
#include "dirent.h"



// A linked list (LL) node to store a queue entry 
struct QNode {
	char key[PATH_MAX + 1];
	struct QNode* next;
};

// The queue, front stores the front node of LL and rear stores the 
// last node of LL 
struct Queue {
	struct QNode* front, * rear;
	pthread_mutex_t lock,
					lock_search,
					lock_finnish ;
	
	pthread_cond_t	not_empty,
					not_ready,
					not_finnish;				

	atomic_int		threads_error_counter,
					idle_threads_counter,
					is_ready,
					is_finnish,
					num_of_founds,
					alive_threads;
	
	char			search_term[PATH_MAX + 1];
	pthread_t*		threads;
	int				num_threads;


};

void handleError(struct Queue* q);
int isDots(char* str);
char* pathcat(char* start_path, char* end_path);
void* search(void* q_);
void wait_for_threads(struct Queue* q);
struct QNode* newNode(char* k);
struct Queue* createQueue(int num_of_threads_, char* search_term_);
int isQEmpty(struct Queue* q);
void enQueue(struct Queue* q, char* k);
int deQueue(struct Queue* q, char* pop_path);
void destroyQueue(struct Queue* q);

// A utility function to create a new linked list node. 
struct QNode* newNode(char* k)
{
	struct QNode* temp = (struct QNode*)malloc(sizeof(struct QNode));
	strcpy(temp->key, k);
	temp->next = NULL;
	return temp;
}

// A utility function to create an empty queue 
struct Queue* createQueue(int num_of_threads_, char* search_term_)
{

	struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
	int rc = pthread_mutex_init(&(q->lock), NULL);
	if (rc) {
		printf("ERROR in pthread_mutex_init(): "
			"%s\n",
			strerror(rc));
		exit(-1);
	}
	rc = pthread_mutex_init(&(q->lock_search), NULL);
	if (rc) {
		printf("ERROR in pthread_mutex_init(): "
			"%s\n",
			strerror(rc));
		exit(-1);
	}
	rc = pthread_mutex_init(&(q->lock_finnish), NULL);
	if (rc) {
		printf("ERROR in pthread_mutex_init(): "
			"%s\n",
			strerror(rc));
		exit(-1);
	}
	rc = pthread_cond_init(&(q->not_empty), NULL);
	if (rc) {
		printf("ERROR in pthread_cond_init(): "
			"%s\n",
			strerror(rc));
		exit(-1);
	}
	rc = pthread_cond_init(&(q->not_ready), NULL);
	if (rc) {
		printf("ERROR in pthread_cond_init(): "
			"%s\n",
			strerror(rc));
		exit(-1);
	}
	rc = pthread_cond_init(&(q->not_finnish), NULL);
	if (rc) {
		printf("ERROR in pthread_cond_init(): "
			"%s\n",
			strerror(rc));
		exit(-1);
	}
	q->front = q->rear = NULL;	
	q->alive_threads = num_of_threads_;
	q->num_threads = num_of_threads_;
	strcpy(q->search_term, search_term_);
	q->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_of_threads_);

	return q;
}

int isQEmpty(struct Queue* q) {
	return(q->front == NULL);
}

// The function to add a key k to q 
void enQueue(struct Queue* q, char* k)
{
	errno = 0;

	pthread_mutex_lock(&q->lock);
	if (errno != 0) {
		handleError(q);
		return;
	}

	// Create a new LL node 
	struct QNode* temp = newNode(k);

	// If queue is empty, then new node is front and rear both 
	if (q->rear == NULL) {
		q->front = q->rear = temp;
	}
	else {
		// Add the new node at the end of queue and change rear 
		q->rear->next = temp;
		q->rear = temp;
	}
	pthread_cond_signal(&(q->not_empty));
	if (errno != 0) {
		handleError(q);
		return;
	}
	pthread_mutex_unlock(&q->lock);
	if (errno != 0) {
		handleError(q);
		return;
	}
}

// Function to remove a key from given queue q 
int deQueue(struct Queue* q , char* pop_path)
{
	errno = 0;
	pthread_mutex_lock(&q->lock);
	if (errno != 0) {
		handleError(q);
		return false;
	}
	while (isQEmpty(q)&& q->is_finnish == 0) {
		(q->idle_threads_counter)++;
		pthread_cond_broadcast(&(q->not_finnish));
		pthread_cond_wait(&(q->not_empty), &(q->lock));	
		(q->idle_threads_counter)--;
	}
	struct QNode* temp = q->front;

	// If queue is empty, return NULL. 
	if (!(q->front == NULL))
		q->front = q->front->next;

	// If front becomes NULL, then change rear also as NULL 
	if (q->front == NULL)
		q->rear = NULL;
	pthread_mutex_unlock(&q->lock);
	if (errno != 0) {
		handleError(q);
		return 1;
	}
	
	if (temp == NULL) {	
		return 1 ;
	}
	strcpy(pop_path, temp->key);
	free(temp);
	return 0;
}
//destroy queue q and all the pthread items
void destroyQueue(struct Queue* q)
{
	errno = 0;
	(q->is_finnish)++;
	char path[PATH_MAX + 1];
	while (!(deQueue(q,path))  && errno == 0) {}
	for(int i = 0 ; i< q->num_threads;i++){
		pthread_cancel((q->threads[i]));
	}
	free(q->threads);
	pthread_mutex_destroy(&(q->lock));
	pthread_mutex_destroy(&(q->lock_search));
	pthread_mutex_destroy(&(q->lock_finnish));
	pthread_cond_destroy(&(q->not_ready));
	pthread_cond_destroy(&(q->not_finnish));
	pthread_cond_destroy(&(q->not_empty));
	free(q);
}

void handleError(struct Queue* q) {
	printf("%s\n", strerror(errno));
	(q->threads_error_counter)++;
	pthread_mutex_lock(&(q->lock_finnish));
	pthread_cond_broadcast(&(q->not_finnish));
	pthread_mutex_unlock(&(q->lock_finnish));
}

int isDots(char* str) {
	char dot[2] = ".";
	char dots[3] = "..";
	return (strcmp(str, dot) == 0 || strcmp(str, dots) == 0);
}
char* pathcat(char* start_path, char* end_path) {
	char slash[2] = "/";
	char* new_path = (char*)malloc(sizeof(char) * (PATH_MAX + 1));
	strcpy(new_path, start_path);
	strcat(new_path, slash);
	strcat(new_path, end_path);
	return new_path;
}
void* search(void* q_) {
	errno = 0;
	struct Queue* q = (struct Queue*)q_;	
	pthread_mutex_lock(&q->lock_search);
	if (errno != 0) {
		handleError(q);
		pthread_exit(NULL);
	}
	//waiting for signal from main thread to start searching
	while (!(q->is_ready)) {
		pthread_cond_wait(&(q->not_ready), &(q->lock_search));
	}
	pthread_mutex_unlock(&q->lock_search);	
	if (errno != 0) {
		handleError(q);
		pthread_exit(NULL);
	}
//====starting the seatrch==========
	char path[PATH_MAX + 1] ;	
	while (!(deQueue(q, path))) {	
		struct dirent* dp;
		struct stat sb;
		DIR* dir = opendir(path);
		DIR* tmp_dir;
		char* new_path;
		if (dir) {
			//Directory exists.
			while ((dp = readdir(dir))) // if dp is null, there's no more content to read
			{
				if (isDots(dp->d_name)) {
					continue;
				}
				new_path = pathcat(path, dp->d_name);//new_path is the new full path
				lstat(new_path, &sb);				
				if (S_ISDIR(sb.st_mode)) {//is a directory
					if (S_ISLNK(sb.st_mode)) {	//is a symbolic link					
					continue;
					}
					// checking for permitions as describe in the cource forum;
					tmp_dir = opendir(new_path);
					if (tmp_dir) {
						closedir(tmp_dir);	
						enQueue(q, new_path);
						continue;
					}
					else{
						printf("Directory %s: Permission denied.\n", new_path);
						continue;
					}						
				}	
				//comparing file to search item
				if (strstr(dp->d_name, q->search_term)) {					
					(q->num_of_founds)++;					
					printf("%s\n", new_path);
				}				
			}
			closedir(dir);
		}			
		
	}
	if (errno != 0) {
		handleError(q);
		pthread_exit(NULL);
	}
	pthread_exit(NULL);
}
//sincronaizing sleeping threads and threads with errors
void wait_for_threads(struct Queue* q) {
	pthread_mutex_lock(&(q->lock_finnish));
	while (!(q->is_finnish)) {
		// all threads had errors
		if (q->threads_error_counter == q->num_threads) {
			(q->is_finnish)++;
			continue;
		}

		// all threads are sleeping
		if (q->idle_threads_counter == q->num_threads && isQEmpty(q)) {
			(q->is_finnish)++;
			continue;
		}
		// Wait until a thread goes to sleep
		pthread_cond_wait(&q->not_finnish, &(q->lock_finnish));		
	}
	pthread_mutex_unlock(&(q->lock_finnish));
}

//argv[1]: search root directory 
//argv[2]: search term
//argv[3] : number of searching threads to be used for the search
int main(int argc, char* argv[]) {
	
	//validate arguments:
	if (argc != 4) {
		printf("need 3 arguments\n");
		exit(EXIT_FAILURE);
	}
	int num_of_threads = atoi(argv[3]);
	if (num_of_threads <= 0) {
		printf("invalid number of threads\n");
		exit(EXIT_FAILURE);
	}
	DIR* dir = opendir(argv[1]);
	if(dir){
		closedir(dir);
	}
	else if (ENOENT == errno) {
		// Directory does not exist.
		printf(" Directory %s does not exist. error %s\n", argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	}
	else {
		// opendir() failed for some other reason.
		printf(" failed to open %s. error %s\n", argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	}

//---------------------------------------------------------------------------------
	// Create a FIFO queue that holds directories:
	struct Queue* q = createQueue(num_of_threads, argv[2]);

//---------------------------------------------------------------------------------
	//Put the search root directory:
	enQueue(q, argv[1]);

//---------------------------------------------------------------------------------
	//Create n searching threads
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	errno = 0;
	for (int i = 0; i < num_of_threads; ++i) {
		pthread_create(&(q->threads[i]), &attr, search, (void*)q);
		if (errno != 0) {
			handleError(q);
			exit(EXIT_FAILURE);
		}
	}

//---------------------------------------------------------------------------------
	//Wait for all other searching threads to be created 
	(q->is_ready)++;
	pthread_cond_broadcast(&(q->not_ready));
	if (errno != 0) {
		handleError(q);
		exit(EXIT_FAILURE);
	}

//---------------------------------------------------------------------------------
	
	wait_for_threads(q);

	printf("Done searching, found %d files\n", q->num_of_founds);

	pthread_attr_destroy(&attr);
	if (errno != 0) {
		exit(EXIT_FAILURE);
	}
	destroyQueue(q);
	if (errno != 0) {
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);

}
