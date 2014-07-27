/********************************************************************************
 * Producer - Consumer problem implemented for 1 producer and multiple consumers
 *
 ***/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#define DBG    0

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
int idxProduce, idxConsume, queueSize, numConsumers;
int producerDone, done;
int sleepProd = 1, sleepCons = 2;
char *circQueue;

// Locks
struct my_rwlock_t lockProduce, lockConsume;

int main (int argc, char* argv[])
{
  if(argc != 3) {
    printf("PROGRAM - No args given\n");
    printf("Usage:\t%s QueueSize NumConsumers\n", argv[0]);
  }
  printf("PROGRAM - START\n");

  /**
  * Intializations
  */
  queueSize = (int) atoi(argv[1]);
  numConsumers = (int) atoi(argv[2]);
  producerDone = 0;
  done = 0;
  printf("PARAMS - Queue Size: %d, Consumers: %d\n", queueSize, numConsumers);
  my_rwlock_init(&lockProduce);
  my_rwlock_init(&lockConsume);

  /**
  * Create producer and consumer threads
  */
  // TODO


  /**
  * Join producer and consumer threads
  */
  // TODO

  printf("PROGRAM - END\n");
}

/******************************************************************************
 * Producer and producer helper functions
 *
 * Based on CS5060/Rao > Lec 6 > Slide 21
 ***/
void *producer(void *producer_thread_data) {
  int i;
  int maxStrSize = 1024*1024;  // Arbitrary max
  FILE *fr;
  char *line;
  /**
   * Working loop
   *
   * Read each character from "string.txt", one-by-one
   * Sequentially write each character into the shared circular queue
   */
  printf("PRODUCER - START\n");
  fr = fopen("string.txt", "rt");
  while(fgets(line, maxStrSize, fr) != NULL) {
    i = 0;
    while(line[i] != '\n') {
      if(enqueue(line[i]))
        i++;
      sleep(sleepProd);
    } // each char in line
  } // each line in file

  setProducerDone();
  printf("PRODUCER - END\n");
}

/******************************************************************************
 * Consumer
 *
 * Based on CS5060/Rao > Lec 6 > Slide 21
 ***/
void *consumer(void *consumer_thread_data) {
  char ch;
  /**
   * Working loop
   *
   * Read and print a character from the shared circular queue
   */
  printf("CONSUMER %ld - START\n", pthread_self());
  while(!isDone()) {
    ch = dequeue();
    if(ch != '\n')
      printf("%c",ch);
  }
  printf("CONSUMER %ld - END\n", pthread_self());
}

/******************************************************************************
 * Global state
 ***/
void setProducerDone() {
  // TODO - write lock done
  producerDone = 1;
}

int isProducerDone() {
  // TODO - read lock
  return producerDone;
}

void setDone() {
  // TODO - write lock done
  done = 1;
}

int isDone() {
  // TODO - read lock done
  return done;
}

/******************************************************************************
 * Queue methods
 ***/
 // Returns success -- failure (1) means you must wait and try again
int enqueue(char ch) {
  // TODO: read lock idxProduce
  // TODO: read lock idxConsume
  if( (idxProduce+1) % queueSize == idxConsume ) {
    // Enqueueing right now would over-write data that has NOT been consumed yet
    return 1;
  }

  // TODO: write lock idxProduce
  idxProduce = (idxProduce+1) % queueSize;
  circQueue[idxProduce] = ch;
  return 0;
}

// Returns a character from the queue; if '\n' the queue was empty, try again later
char dequeue() {
  // TODO: read lock idxProduce
  // TODO: read lock idxConsume
  if( (idxConsume+1) % queueSize == idxProduce ) {
    // Queue is empty!
    return '\n';
  }

  // TODO: write lock idxConsume
  idxConsume = (idxConsume+1) % queueSize;
  return circQueue[idxConsume];
}

/******************************************************************************
 * Read-Write Lock base functions
 *
 * From CS5060/Rao > Lec 6 > Slides 37-38
 ***/
void my_rwlock_init(my_rwlock_t *l) {
   printf("INIT RW_LOCK\n");
   l->readers = l->writer = l->pending_writers = 0;
   pthread_mutex_init(&(l->read_write_lock), NULL);
   pthread_cond_init(&(l->writer_proceed), NULL);
}

void my_rwlock_rlock(my_rwlock_t *l) {
   /**
   * If there's a write lock or pending writers, wait
   * Else increment the count of readers and grant read lock
   */
   printf("R_LCK - START\n");
   pthread_mutex_lock(&(l->read_write_lock));
   while((l->pending_writers > 0) || (l->writer > 0))
      pthread_cond_wait(&(l->readers_proceed), &(l->read_write_lock));
   l->readers++;     // I'm also reading now
   pthread_mutex_unlock(&(l->read_write_lock));
   printf("R_LCK - END\n");
}

void my_rwlock_wlock(my_rwlock_t *l) {
   /**
    * If there are readers or writers, increment pending writers count and wait
    *
    * Once woken, decrement pending writers count and increment writer count
    */
   printf("W_LCK - START\n");
   pthread_mutex_lock(&(l->read_write_lock));
   while((l->writer > 0) || (l->readers > 0)) {
      l->pending_writers++;
      pthread_cond_wait(&(l->writer_proceed), &(l->read_write_lock));
   }
   l->pending_writers--;
   l->writer++;
   pthread_mutex_unlock(&(l->read_write_lock));
   printf("W_LCK - START\n");
}

void my_rwlock_unlock(my_rwlock_t *l) {
   /**
    * If there's a write lock, then unlock
    * Else if there are read locks, decrement the read count
    *
    * If the count is 0 and a pending writer, let it go through
    * Else if pending readers, let them all go through
    */
   printf("UNLCK - START\n");
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
   printf("UNLCK - START\n");
}
/***
 * End Read-Write Lock base functions
 *****************************************************************************/