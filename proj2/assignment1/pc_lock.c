/********************************************************************************
 * Producer - Consumer problem implemented for 1 producer and multiple consumers
 *
 ***/
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define DBG    0
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
   int queueCapacity;               // count of the number of *available* slots in q
   int queueCount;                  // count of the number of *current* slots in q
   int auditWrites;
   int auditReads;
   int pending_readers;             // number of pending readers
   int pending_writers;             // number of pending writers
   pthread_mutex_t read_write_lock; // mutex lock for the shared data structure
};

/******************************************************************************
 * Function declarations
 ***/
void *producer(void *producer_thread_data);
void *consumer(void *consumer_thread_data);
void my_rwlock_init(my_rwlock_t *l, int queueSize);
void my_rwlock_rlock(my_rwlock_t *l);
void my_rwlock_wlock(my_rwlock_t *l);
void my_rwlock_unlock(my_rwlock_t *l);
void my_rwlock_set_proceed(my_rwlock_t *l);
int room_to_write(my_rwlock_t *l);
int room_to_read(my_rwlock_t *l);
void setProducerDone();
int isProducerDone();
void setDone();
int isDone();
void enqueue(char ch);
char dequeue();
void printQueue();

/******************************************************************************
 * Shared variables and Main function
 ***/
char *circQueue;
int idxProduce=0;   // Point to the next destination
int idxConsume=0;   // Point to the next destination
int queueSize, numConsumers, done;
int sleepProd = 1000*10, sleepCons = 2000*10;

// Locks
struct my_rwlock_t lockQueue;
pthread_mutex_t lockDone, lockConsumeIdx;

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
  pthread_mutex_init(&lockConsumeIdx, NULL);
  pthread_mutex_init(&lockDone, NULL);
  done = 0;
  if(DBG) printf("PARAMS - Queue Size: %d, Consumers: %d\n", queueSize, numConsumers);
  my_rwlock_init(&lockQueue, queueSize);

  /**
  * Create producer and consumer threads
  */
  threads = malloc( (numConsumers+1) * sizeof(pthread_t) );
  // Producer thread
  if(rc = pthread_create(&threads[0], NULL, producer, NULL))
    if(DBG) printf("Error creating producer thread\n");

  // Sleep to ensure producer starts first
  usleep(sleepProd);

  // Create a thread per consumer
  for(i=1; i < (numConsumers+1); i++) {
    if(rc = pthread_create(&threads[i], NULL, consumer, (void*)0))
      if(DBG) printf("Error creating consumer thread %d\n", i);
  } // create each consumer

  if(DBG) printf("MAIN - Created threads: [");
  for(i=0; i<(numConsumers+1);i++) {
    if(DBG) printf("%ld, ", threads[i]);
  }
  if(DBG) printf("]\n");

  //
  // They're doing there thing, hold on...
  //

  /**
  * Join producer and consumer threads
  */
  for(i=1; i < 2; i++) {
    if(DBG) printf("MAIN - wait to join thread %d\n", i);
    pthread_join(threads[i], NULL);
    if(DBG) printf("MAIN - joined thread %d\n", i);
  } // join each thread

  if(DBG) printf("\nPROGRAM - END\n");
  //pthread_exit(NULL);
  return 0;
}

/******************************************************************************
 * Producer and producer helper functions
 *
 * Based on CS5060/Rao > Lec 6 > Slide 21
 ***/
void * producer(void *producer_thread_data) {
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
  if(DBG) printf("[%ld] PRODUCER - START\n", pthread_self());
  fr = fopen("string.txt", "rt");
  while(fgets(line, maxStrSize, fr) != NULL) {
    ttl = 10; // It will only loop in place TTL times
    if(DBG) printf("[%ld] PRODUCER - LINE = '%s'\n", pthread_self(), line);

    for(i = 0; i < strlen(line) && line[i] != '\n' && ttl > 0; ttl--) {

      if(DBG) printf("[%ld] PRODUCER - Q > Get write lock\n", pthread_self());
      my_rwlock_wlock(&lockQueue);
      if(DBG) printf("[%ld] PRODUCER - Q > Have write lock\n", pthread_self());

      enqueue(line[i]);
      i++;
      ttl = 20;

      if(DBG) printf("[%ld] PRODUCER - Q > Release r/w lock\n", pthread_self());
      my_rwlock_unlock(&lockQueue);
      if(DBG) printf("[%ld] PRODUCER - Q > Released r/w lock\n", pthread_self());
      // usleep(sleepProd);
    } // each char in line
  } // each line in file

  setDone();
  
  if(DBG) printf("[%ld] PRODUCER - END\n", pthread_self());
  usleep(sleepProd);
  if(DBG) printf("PRODUCER - Slept\n");
  //pthread_exit(NULL);
  return (void*) 0;
}

/******************************************************************************
 * Consumer
 *
 * Based on CS5060/Rao > Lec 6 > Slide 21
 ***/
void * consumer(void *consumer_thread_data) {
  int imDone = 0;
  char ch;
  /**
   * Working loop
   *
   * Read and print a character from the shared circular queue
   */
  if(DBG) printf("[%ld] CONSUMER - START\n", pthread_self());
  while(!imDone) {
    if(DBG) printf("[%ld] CONSUMER - Q > Get read lock\n", pthread_self());
    my_rwlock_rlock(&lockQueue);
    if(DBG) printf("[%ld] CONSUMER - Q > Have read lock\n", pthread_self());

    if(DBG) printf("[%ld] CONSUMER - I > Get mutex\n", pthread_self());
    pthread_mutex_lock(&lockConsumeIdx);
    if(DBG) printf("[%ld] CONSUMER - I > Have mutex\n", pthread_self());
    ch = '\n';
    if(!isDone() || (idxConsume < idxProduce))
      ch = dequeue();
    if(DBG) printf("[%ld] CONSUMER - I > Release mutex\n", pthread_self());
    pthread_mutex_unlock(&lockConsumeIdx);

    // Double check
    if(ch != '\n') {
      if(DBG) {
        printf("[%ld] CONSUMER - DEQ '%c'\n", pthread_self(), ch);
        printf("\t\t\t\t\t\t\t>>>>>>>>>>>>>> '%c'\n", ch);
      } else {
        printf("%c", ch);
      }
    }

    if(DBG) printf("[%ld] CONSUMER - Q > Release r/w lock\n", pthread_self());
    my_rwlock_unlock(&lockQueue);
    if(DBG) printf("[%ld] CONSUMER - Q > Released r/w lock\n", pthread_self());
  }

  if(DBG) printf("[%ld] CONSUMER - END\n", pthread_self());
  //pthread_exit(NULL);
  return (void*) 0;
}

/******************************************************************************
 * Global state
 ***/
void setDone() {
  pthread_mutex_lock(&lockDone);
  done = 1;
  printf(" [DONE!]\n");
  pthread_mutex_unlock(&lockDone);
  my_rwlock_set_proceed(&lockQueue);
}

int isDone() {
  int localDone;
  pthread_mutex_lock(&lockDone);
  localDone = done;
  pthread_mutex_unlock(&lockDone);
  return localDone;
}

/******************************************************************************
 * Queue methods
 ***/
 // Returns success -- failure (0) means you must wait and try again
void enqueue(char ch) {
  circQueue[idxProduce] = ch;
  if(DBG) printf("[%d] ENQ > '%c'\n", idxProduce, ch);
  idxProduce = (idxProduce+1) % queueSize;
  return;
}

// Returns a character from the queue; if '\n' the queue was empty, try again later
char dequeue() {
  char ch;
  ch = circQueue[idxConsume];
  if(DBG) printf("[%d] DEQ < '%c'\n", idxConsume, ch);
  idxConsume = (idxConsume+1) % queueSize;
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
 * Modified to maintain knowledge of the queue's state (empty, not empty, not full, full)
 * Key assumptions:
 *    - Queue capacity is larger than the number of readers
 *    - Queue capacity is also larger than number of writers
 *
 * From CS5060/Rao > Lec 6 > Slides 37-38
 ***/
void my_rwlock_init(my_rwlock_t *l, int queueSize) {
   if(DBG) printf("INIT RW_LOCK\n");
   l->readers = l->writer = l->pending_writers = l->queueCount = 0;
   l->queueCapacity = queueSize;
   l->auditWrites = l->auditReads = 0;
   pthread_mutex_init(&(l->read_write_lock), NULL);
   pthread_cond_init(&(l->writer_proceed), NULL);
}

void my_rwlock_rlock(my_rwlock_t *l) {
  /**
  * If there's a write lock or pending writers, wait
  * Else increment the count of readers and grant read lock
  */
  pthread_mutex_lock(&(l->read_write_lock));
  while((l->pending_writers > 0 && room_to_write(l)) || (l->writer > 0) || !room_to_read(l)) {
    if(!room_to_read(l) && room_to_write(l)) {
      if(DBG) printf("[%ld] Signal 'writer_proceed'\n",pthread_self());
      pthread_cond_signal(&(l->writer_proceed));
    }
    l->pending_readers++;
    if(DBG) printf("[%ld] Wait for 'readers_proceed'\n",pthread_self());
    pthread_cond_wait(&(l->readers_proceed), &(l->read_write_lock));
    if(DBG) printf("[%ld] Got 'readers_proceed'\n",pthread_self());
    l->pending_readers--;
    if(isDone() && l->queueCount == 0 ){
      if(DBG) printf("[%ld] I've got better things to do than wait for nothing to happen.'\n",pthread_self());
      //pthread_exit(NULL);
      //return (void*) 0;
    }
  }
  l->readers++;     // I'm also reading now
  //proactivel decrementing the queueCount assuming the reader will remove
  l->queueCount--; // I was a reader, so assume something was read
  if(DBG) printf("CUR READERS: %d\n", l->readers);
  pthread_mutex_unlock(&(l->read_write_lock));
}

void my_rwlock_wlock(my_rwlock_t *l) {
  /**
   * If there are readers or writers, increment pending writers count and wait
   *
   * Once woken, decrement pending writers count and increment writer count
   */
  pthread_mutex_lock(&(l->read_write_lock));
  while((l->writer > 0) || (l->readers > 0) || !room_to_write(l)) {
    if(l->pending_readers > 0) {
      if(room_to_read(l)) {
        if(DBG) printf("[%ld] Broadcast 'readers_proceed'\n",pthread_self());
        pthread_cond_broadcast(&(l->readers_proceed));
      } else if(l->queueCount > 0) {
        if(DBG) printf("[%ld] Signal 'readers_proceed'\n",pthread_self());
        pthread_cond_signal(&(l->readers_proceed));
      }
    }

    l->pending_writers++;
    if(DBG) printf("[%ld] Wait for 'writer_proceed'\n",pthread_self());
    pthread_cond_wait(&(l->writer_proceed), &(l->read_write_lock));
    if(DBG) printf("[%ld] Got 'writer_proceed'\n",pthread_self());
    l->pending_writers--;
  }
  l->writer++;
  //proactively increment queueCount assuming writer will add to queue
  l->queueCount++; // I was a writer, so assume something was written
  pthread_mutex_unlock(&(l->read_write_lock));
}

void my_rwlock_unlock(my_rwlock_t *l) {
  int roomToWrite, roomToRead, letWritersGo, letAllReadersGo, letOneReaderGo;
  /**
   * If there's a write lock, then unlock
   * Else if there are read locks, decrement the read count
   *
   * If the count is 0 and a pending writer, let it go through
   *
   * *** Modification to optimize for this problem:
   * (ORIGINAL) Else if pending readers, let them all go through
   * (MODIFIED) Else if pending readers > available items
   */
  pthread_mutex_lock(&(l->read_write_lock));
  if(l->writer > 0) {
    l->writer = 0;
    l->auditWrites++;
  } else if(l->readers > 0) {
    l->readers--;
    l->auditReads++;
  }
  if(DBG) printf("\t\t\t\t\t\t\tAUDIT ==> Cnt: %d, Writes: %d, Reads: %d\n", l->queueCount, l->auditWrites, l->auditReads);
  roomToWrite = room_to_write(l);
  roomToRead = room_to_read(l);
  letWritersGo = (l->readers == 0) && (l->pending_writers > 0 && roomToWrite);
  letAllReadersGo = l->pending_readers > 0 && roomToRead;
  letOneReaderGo = l->pending_readers > 0 && l->queueCount > 0;
  pthread_mutex_unlock(&(l->read_write_lock));

  // Let the writers proceed if there are no readers, AND there's enough space for all the writers
  if(letWritersGo)
  {
    if(DBG) printf("UNLOCK... Signal writer_proceed\n");
    pthread_cond_signal(&(l->writer_proceed));
  }
  // ORIGINAL
  // else if(l->readers > 0)
  // MODIFIED
  else if(letAllReadersGo)
  {
    if(DBG) printf("UNLOCK... Broadcast readers_proceed\n");
    pthread_cond_broadcast(&(l->readers_proceed));
  }
  else if(letOneReaderGo)
  {
    if(DBG) printf("UNLOCK... Signal readers_proceed\n");
    pthread_cond_signal(&(l->readers_proceed));
  }
}

void my_rwlock_set_proceed(my_rwlock_t *l) {
  pthread_cond_broadcast(&(l->readers_proceed));
}

int room_to_write(my_rwlock_t *l) {
  if(DBG) printf("Room to write? (cnt=%d, cap=%d)  ", l->queueCount, l->queueCapacity);
  if(l->queueCount < l->queueCapacity) {
    if(DBG) printf("Yes\n");
    return 1;
  } else {
    if(DBG) printf("No\n");
    return 0;
  }
}
int room_to_read(my_rwlock_t *l) {
  if(DBG) printf("Room to read? (cnt=%d, cap=%d, readers=%d, pending_readers=%d)  ", l->queueCount, l->queueCapacity, l->readers, l->pending_readers);
  if(l->queueCount >= (l->readers + l->pending_readers) && (l->readers + l->pending_readers) <= l->queueCount) {
    if(DBG) printf("Yes\n");
    return 1;
  } else {
    if(DBG) printf("No\n");
    return 0;
  }
}
/***
 * End Read-Write Lock base functions
 *****************************************************************************/
