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

/******************************************************************************
 * Shared variables and Main function
 ***/
int idxProduce=0, idxConsume=-1, queueSize, numConsumers, done;
char *circQueue;

// Locks
struct my_rwlock_t lockProduce, lockConsume;
pthread_mutex_t lockDone;

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
  done = 0;
  if(DBG) printf("PARAMS - Queue Size: %d, Consumers: %d\n", queueSize, numConsumers);
  my_rwlock_init(&lockProduce);
  my_rwlock_init(&lockConsume);

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
    pthread_join(threads[i], NULL);
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
    ttl = 20;
    if(DBG) printf("PRODUCER - LINE = '%s'\n", line);
    for(i = 0; i < strlen(line[i]); ttl--) {
      if(enqueue(line[i])) {
        if(DBG) printf("PRODUCER - ENQ '%c'\n", line[i]);
        i++;
        ttl = 20;
      }
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
    ch = dequeue();
    if(ch != '\n'){
      if(DBG) printf("CONSUMER %ld - DEQ '%c'\n", pthread_self(), ch);
      if(!DBG) printf("%c", ch);
    }
    // If we've caught up to the producer, and the producer is done,
    //    then I'M DONE!
    else if(isDone())
      imDone = 1;
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
 // Returns success -- failure (1) means you must wait and try again
int enqueue(char ch) {
  my_rwlock_rlock(&lockProduce);
  my_rwlock_rlock(&lockConsume);
  if( (idxProduce+1) % queueSize == idxConsume ) {
    // Enqueueing right now would over-write data that has NOT been consumed yet
    my_rwlock_unlock(&lockConsume);
    my_rwlock_unlock(&lockProduce);
    return 1;
  }
  my_rwlock_unlock(&lockConsume);
  my_rwlock_unlock(&lockProduce);

  my_rwlock_wlock(&lockProduce);
  idxProduce = (idxProduce+1) % queueSize;
  circQueue[idxProduce] = ch;
  my_rwlock_unlock(&lockProduce);
  return 0;
}

// Returns a character from the queue; if '\n' the queue was empty, try again later
char dequeue() {
  char ch = '\n';
  my_rwlock_rlock(&lockConsume);
  my_rwlock_rlock(&lockProduce);
  if( (idxConsume+1) % queueSize == idxProduce ) {
    // Queue is empty!
    my_rwlock_unlock(&lockProduce);
    my_rwlock_unlock(&lockConsume);
    return ch;
  }
  my_rwlock_unlock(&lockProduce);
  my_rwlock_unlock(&lockConsume);

  my_rwlock_wlock(&lockConsume);
  idxConsume = (idxConsume+1) % queueSize;
  ch = circQueue[idxConsume];
  my_rwlock_unlock(&lockConsume);
  return ch;
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
   if(DBG) printf("R_LCK - START\n");
   pthread_mutex_lock(&(l->read_write_lock));
   while((l->pending_writers > 0) || (l->writer > 0))
      pthread_cond_wait(&(l->readers_proceed), &(l->read_write_lock));
   l->readers++;     // I'm also reading now
   pthread_mutex_unlock(&(l->read_write_lock));
   if(DBG) printf("R_LCK - END\n");
}

void my_rwlock_wlock(my_rwlock_t *l) {
   /**
    * If there are readers or writers, increment pending writers count and wait
    *
    * Once woken, decrement pending writers count and increment writer count
    */
   if(DBG) printf("W_LCK - START\n");
   pthread_mutex_lock(&(l->read_write_lock));
   while((l->writer > 0) || (l->readers > 0)) {
      l->pending_writers++;
      pthread_cond_wait(&(l->writer_proceed), &(l->read_write_lock));
   }
   l->pending_writers--;
   l->writer++;
   pthread_mutex_unlock(&(l->read_write_lock));
   if(DBG) printf("W_LCK - START\n");
}

void my_rwlock_unlock(my_rwlock_t *l) {
   /**
    * If there's a write lock, then unlock
    * Else if there are read locks, decrement the read count
    *
    * If the count is 0 and a pending writer, let it go through
    * Else if pending readers, let them all go through
    */
   if(DBG) printf("UNLCK - START\n");
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
   if(DBG) printf("UNLCK - START\n");
}
/***
 * End Read-Write Lock base functions
 *****************************************************************************/