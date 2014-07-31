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

int main(int argc, char* argv[]) {
  if(DBG) printf("Start program\n");

  #pragma omp parallel num_threads (2) shared (buffer) shared (taskAvailable)
  {
    printf("[%ld] Entering parallel\n", pthread_self());

    // Producer goes here

    // Consumer goes here

    printf("[%ld] Exiting parallel\n", pthread_self());
  }

  if(DBG) printf("End program\n");
}