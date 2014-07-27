/********************************************************************************
 * Producer - Consumer problem implemented for 1 producer and multiple consumers
 *
 ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define DBG    0

/******************************************************************************
 * Read-Write Lock Structure
 *
 * From CS5060/Rao > Lec 6 > Slide 36
 ***/
typedef struct {
   int readers;                     // count of the number of readers
   int writer;                      // the writer (0/1 int specifying presence)
   pthread_cond_t readers_proceed;  // signaled when readers can proceed
   pthread_cond_t writer_proceed;   // signaled when one of the writers can proceed
   int pending_writers;             // number of pending writers
   pthread_mutex_t read_write_lock; // mutex lock for the shared data structure
} my_rwlock_t;

int main (int argc, char* argv[])
{
   printf("PROGRAM - START\n");

   printf("PROGRAM - END\n");
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