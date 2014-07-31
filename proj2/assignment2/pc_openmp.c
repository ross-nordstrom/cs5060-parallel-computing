/********************************************************************************
 * Producer - Consumer problem with 1 producer and 1 consumer, using OpenMP
 *
 ***/
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define DBG    1
#define DBG2   0

char buffer = '\n';
int taskAvailable = 0;
int done = 0;

int main(int argc, char* argv[]) {
  if(DBG) printf("Start program\n");

  #pragma omp parallel num_threads (2) shared (buffer) shared (taskAvailable) shared (done)
  {
    if(DBG) printf("[%ld] Entering parallel\n", pthread_self());

    #pragma omp sections
    {
      #pragma omp section
      {
        // Producer goes here
        if(DBG) printf("[%ld] Entering section -- Producer\n", pthread_self());
        sleep(1);
        done = 1;
        if(DBG) printf("[%ld] Exiting section -- Producer\n", pthread_self());
      } // omp section - producer

      #pragma omp section
      {
        // Consumer goes here
        if(DBG) printf("[%ld] Entering section -- Consumer\n", pthread_self());
        while(!done) {
          if(DBG) printf("[%ld] Not done yet...\n", pthread_self());
          usleep(100000);
        }
        if(DBG) printf("[%ld] Done.\n", pthread_self());
        if(DBG) printf("[%ld] Exiting section -- Consumer\n", pthread_self());
      } // omp section - consumer

    } // omp sections
    if(DBG) printf("[%ld] Exiting parallel\n", pthread_self());
  } // omp parallel

  if(DBG) printf("End program\n");
}