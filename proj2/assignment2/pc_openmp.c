/********************************************************************************
 * Producer - Consumer problem with 1 producer and 1 consumer, using OpenMP
 *
 ***/
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define DBG    0
#define DBG2   0

char buffer = '\n';
int taskAvailable = 0;
int done = 0;

int main(int argc, char* argv[]) {
  if(DBG) printf("Start program\n");

  #pragma omp parallel sections shared (buffer) shared (taskAvailable) shared (done)
  {
    // if(DBG) printf("[%ld] Entering parallel\n", pthread_self());

    #pragma omp section
    {
      // Producer goes here
      if(DBG) printf("[%ld] Entering section -- Producer\n", pthread_self());

      int i=0, maxStrSize = 1024*1024;  // Arbitrary max
      FILE *fr = NULL;
      char ch, line[1024*1024];

      fr = fopen("string.txt", "rt");
      if(DBG) printf("[%ld] File opened for reading.\n", pthread_self());

      while(fgets(line, maxStrSize, fr) != NULL) {
        if(DBG) printf("[%ld] Read line: '%s'\n", pthread_self(), line);

        i = 0;
        while( (ch = line[i]) != '\n' )
        {
          #pragma omp critical(taskQueue)
          {
            if(!taskAvailable)
            {
              buffer = ch;
              taskAvailable = 1;
              i++;
            }
          }
        } // for each char in line
      } // for each line in file
      fclose(fr);

      #pragma omp critical(isDone)
      {
        done = 1;
      }

      if(DBG) printf("[%ld] Exiting section -- Producer\n", pthread_self());
    } // omp section - producer

    #pragma omp section
    {
      int imDone = 0;
      char ch;

      // Consumer goes here
      if(DBG) printf("[%ld] Entering section -- Consumer\n", pthread_self());
      while(!imDone) {
        #pragma omp critical(taskQueue)
        {
          if(taskAvailable)
          {
            ch = buffer;
            if(DBG) printf("[%ld] Got task from buffer: '%c'\n", pthread_self(), ch);
            else printf("%c",ch);
            taskAvailable = 0;
          }
        }

        #pragma omp critical(isDone)
        {
          imDone = done;
        }
      } // while i'm not done
      if(DBG) printf("[%ld] Done.\n", pthread_self());
      if(DBG) printf("[%ld] Exiting section -- Consumer\n", pthread_self());
      else printf("\n");
    } // omp section - consumer

    // if(DBG) printf("[%ld] Exiting parallel\n", pthread_self());
  } // omp parallel sections

  if(DBG) printf("End program\n");
}