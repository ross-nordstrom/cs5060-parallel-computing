/********************************************************************************
 * Producer - Consumer problem implemented for 1 producer and multiple consumers
 *
 ***/
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define DBG    1
#define DBG2   0

/******************************************************************************
 * Read-Write Lock Structure
 *
 * From CS5060/Rao > Lec 6 > Slide 36
 ***/
typedef struct my_rwlock_t my_rwlock_t;
struct my_rwlock_t {
   int readers;                     // count of the number of readers
   int writer;                      // the writer (0/1 int specifying presence)
   pthread_cond_t readers_proceed;  // signaled when readers can proceed
   pthread_cond_t writer_proceed;   // signaled when one of the writers can proceed
   int pending_writers;             // number of pending writers
   pthread_mutex_t read_write_lock; // mutex lock for the shared data structure
};

/******************************************************************************
 * Function declarations
 ***/
void *producer(void *producer_thread_data);
void *consumer(void *consumer_thread_data);
void my_rwlock_init(my_rwlock_t *l);
void my_rwlock_rlock(my_rwlock_t *l);
void my_rwlock_wlock(my_rwlock_t *l);
void my_rwlock_unlock(my_rwlock_t *l);
void setProducerDone();
int isProducerDone();
void setDone();
int isDone();
int enqueue(char ch);
char dequeue();
void printQueue();

/******************************************************************************
 * Shared variables and Main function
 ***/
int idxProduce=0;   // Point to the next destination
int idxConsume=0;   // Point to the next destination
int itemsInQueue=0;
int queueSize, numConsumers, done;
int sleepProd = 1000*10, sleepCons = 2000*10;
char *circQueue;

// Locks
struct my_rwlock_t lockQueue;
pthread_mutex_t lockDone, lockQueueSize;

int main (int argc, char* argv[])
{
  int i, rc;
  pthread_t *threads;

  if(argc != 3) {
    if(DBG) printf("PROGRAM - No args given\n");
    if(DBG) printf("Usage:\t%s QueueSize NumConsumers\n", argv[0]);
    exit(1);
  }
  if(DBG) printf("PROGRAM - START\n");

  /**
  * Intializations
  */
  queueSize = (int) atoi(argv[1]);
  numConsumers = (int) atoi(argv[2]);
  circQueue = malloc(queueSize*sizeof(char));
  idxConsume = queueSize-1;
  done = 0;
  if(DBG) printf("PARAMS - Queue Size: %d, Consumers: %d\n", queueSize, numConsumers);
  my_rwlock_init(&lockQueue);

  /**
  * Create producer and consumer threads
  */
  threads = malloc( (numConsumers) * sizeof(pthread_t) );
  // Producer thread
  if(rc = pthread_create(&threads[0], NULL, producer, NULL))
    if(DBG) printf("Error creating producer thread\n");

  // Create a thread per consumer
  for(i=0; i < numConsumers; i++) {
    if(rc = pthread_create(&threads[i+1], NULL, consumer, NULL))
      if(DBG) printf("Error creating consumer thread %d\n", i);
  } // create each consumer

  //
  // They're doing there thing, hold on...
  //

  /**
  * Join producer and consumer threads
  */
  for(i=0; i < (numConsumers+1); i++) {
    printf("MAIN - wait to join thread %d\n", i);
    pthread_join(threads[i], NULL);
    printf("MAIN - joined thread %d\n", i);
  } // join each thread

  if(DBG) printf("\nPROGRAM - END\n");
}

/******************************************************************************
 * Producer and producer helper functions
 *
 * Based on CS5060/Rao > Lec 6 > Slide 21
 ***/
void *producer(void *producer_thread_data) {
  int i, ttl;
  int maxStrSize = 1024*1024;  // Arbitrary max
  FILE *fr;
  char *line;

  /**
   * Working loop
   *
   * Read each character from "string.txt", one-by-one
   * Sequentially write each character into the shared circular queue
   */
  if(DBG) printf("PRODUCER - START\n");
  fr = fopen("string.txt", "rt");
  while(fgets(line, maxStrSize, fr) != NULL) {
    ttl = 10; // It will only loop in place TTL times
    if(DBG) printf("PRODUCER - LINE = '%s'\n", line);
    for(i = 0; i < strlen(line) && line[i] != '\n' && ttl > 0; ttl--) {
      if(DBG) printf("PRODUCER - Get write lock\n");
      my_rwlock_wlock(&lockQueue);
      if(DBG) printf("PRODUCER - Have write lock\n");
      if(enqueue(line[i])) {
        if(DBG) printf("PRODUCER - ENQ '%c'\n", line[i]);
        i++;
        ttl = 20;
      }
      if(DBG) printf("PRODUCER - Release write lock\n");
      my_rwlock_unlock(&lockQueue);
    } // each char in line
  } // each line in file

  setDone();
  if(DBG) printf("PRODUCER - END\n");
  pthread_exit(NULL);
}

/******************************************************************************
 * Consumer
 *
 * Based on CS5060/Rao > Lec 6 > Slide 21
 ***/
void *consumer(void *consumer_thread_data) {
  int imDone = 0;
  char ch;
  /**
   * Working loop
   *
   * Read and print a character from the shared circular queue
   */
  if(DBG) printf("CONSUMER %ld - START\n", pthread_self());
  while(!imDone) {
    if(DBG) printf("CONSUMER %ld - Get read lock\n", pthread_self());
    my_rwlock_rlock(&lockQueue);
    if(DBG) printf("CONSUMER %ld - Have read lock\n", pthread_self());
    ch = dequeue();
    if(ch != '\n'){
      if(DBG) {
        printf("CONSUMER %ld - DEQ '%c'\n", pthread_self(), ch);
        printf("\t\t\t>>>>>>>>>>>>>>> '%c'\n", ch);
      } else {
        printf("%c", ch);
      }
    }
    // If we've caught up to the producer, and the producer is done,
    //    then I'M DONE!
    else {
      if(isDone()) {
        imDone = 1;
        if(DBG) printf("CONSUMER %ld - I'M DONE!\n", pthread_self());
      }
    }
    if(DBG) printf("CONSUMER %ld - Release read lock\n", pthread_self());
    my_rwlock_unlock(&lockQueue);
  }
  if(DBG) printf("CONSUMER %ld - END\n", pthread_self());
  pthread_exit(NULL);
}

/******************************************************************************
 * Global state
 ***/
void setDone() {
  pthread_mutex_lock(&lockDone);
  done = 1;
  printf("DONE!\n");
  pthread_mutex_unlock(&lockDone);
}

int isDone() {
  pthread_mutex_lock(&lockDone);
  return done;
  pthread_mutex_unlock(&lockDone);
}

/******************************************************************************
 * Queue methods
 ***/
 // Returns success -- failure (0) means you must wait and try again
int enqueue(char ch) {
  int success = 0;
  if( itemsInQueue != queueSize ) {
    circQueue[idxProduce] = ch;
    if(DBG) printf("[%d] ENQ > '%c'\n", idxProduce, ch);
    idxProduce = (idxProduce+1) % queueSize;
  pthread_mutex_lock(&lockQueueSize);
    itemsInQueue++;
  pthread_mutex_unlock(&lockQueueSize);
    success = 1;
  }
  return success;
}

// Returns a character from the queue; if '\n' the queue was empty, try again later
char dequeue() {
  char ch = '\n';
  if(itemsInQueue != 0) {
    ch = circQueue[idxConsume];
    if(DBG) printf("[%d] DEQ < '%c'\n", idxConsume, ch);
    idxConsume = (idxConsume+1) % queueSize;
  pthread_mutex_lock(&lockQueueSize);
    itemsInQueue--;
  pthread_mutex_unlock(&lockQueueSize);
  }
  return ch;
}

void printQueue() {
  int i;
  my_rwlock_rlock(&lockQueue);
  printf(" [%-10s]\n  ", circQueue);
  my_rwlock_unlock(&lockQueue);
}

/******************************************************************************
 * Read-Write Lock base functions
 *
 * From CS5060/Rao > Lec 6 > Slides 37-38
 ***/
void my_rwlock_init(my_rwlock_t *l) {
   if(DBG) printf("INIT RW_LOCK\n");
   l->readers = l->writer = l->pending_writers = 0;
   pthread_mutex_init(&(l->read_write_lock), NULL);
   pthread_cond_init(&(l->writer_proceed), NULL);
}

void my_rwlock_rlock(my_rwlock_t *l) {
   /**
   * If there's a write lock or pending writers, wait
   * Else increment the count of readers and grant read lock
   */
   if(DBG2) printf("[");
   pthread_mutex_lock(&(l->read_write_lock));
   while((l->pending_writers > 0) || (l->writer > 0))
      pthread_cond_wait(&(l->readers_proceed), &(l->read_write_lock));
   l->readers++;     // I'm also reading now
   pthread_mutex_unlock(&(l->read_write_lock));
   if(DBG2) printf("]");
}

void my_rwlock_wlock(my_rwlock_t *l) {
   /**
    * If there are readers or writers, increment pending writers count and wait
    *
    * Once woken, decrement pending writers count and increment writer count
    */
   if(DBG2) printf("(");
   pthread_mutex_lock(&(l->read_write_lock));
   while((l->writer > 0) || (l->readers > 0)) {
      l->pending_writers++;
      pthread_cond_wait(&(l->writer_proceed), &(l->read_write_lock));
   }
   l->pending_writers--;
   l->writer++;
   pthread_mutex_unlock(&(l->read_write_lock));
   if(DBG2) printf(")");
}

void my_rwlock_unlock(my_rwlock_t *l) {
   /**
    * If there's a write lock, then unlock
    * Else if there are read locks, decrement the read count
    *
    * If the count is 0 and a pending writer, let it go through
    * Else if pending readers, let them all go through
    */
   if(DBG2) printf("<");
   pthread_mutex_lock(&(l->read_write_lock));
   if(l->writer > 0)
      l->writer = 0;
   else if(l->readers > 0)
      l->readers--;
   pthread_mutex_unlock(&(l->read_write_lock));
   if((l->readers == 0) && (l->pending_writers > 0))
      pthread_cond_signal(&(l->writer_proceed));
   else if(l->readers > 0)
      pthread_cond_broadcast(&(l->readers_proceed));
   if(DBG2) printf(">");
}
/***
 * End Read-Write Lock base functions
 *****************************************************************************/