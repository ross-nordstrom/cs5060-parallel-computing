/********************************************************************************
 * Producer - Consumer problem implemented for 1 producer and multiple consumers
 *
 ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define DBG    0

typedef struct {
   int readers;                     // count of the number of readers
   int writer;                      // the writer (0/1 int specifying presence)
   pthread_cond_t readers_proceed;  // signaled when readers can proceed
   pthread_cont_t writer_proceed;   // signaled when one of the writers can proceed
   int pending_writers;             // number of pending writers
   pthread_mutex_t read_write_lock; // mutex lock for the shared data structure
} my_rwlock_t;

int main (int argc, char* argv[])
{
   printf("PROGRAM START\n");

   printf("PROGRAM END\n");
}

void my_rwlock_init(my_rwlock_t *l) {
   printf("INIT RW_LOCK\n");
}

void my_rwlock_rlock(my_rwlock_t *l) {
   printf("RLCK RW_LOCK\n");
}

void my_rwlock_wlock(my_rwlock_t *l) {
   printf("WLCK RW_LOCK\n");
}
