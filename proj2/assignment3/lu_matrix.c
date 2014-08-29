#include <omp.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define MAX_LINE_SIZE 120
#define DBG 0

int readInputFile(double ***A){
  double **atmp;
  FILE *fr;
  double tmp;
  char *line, *tok;
  int n, i, j;
 
  line = malloc(MAX_LINE_SIZE * sizeof(char));

  if(DBG) printf("Entering ReadInputFile\n");

  fr = fopen("data.txt", "rt");
  if(fgets(line, MAX_LINE_SIZE, fr) != NULL) {
    n = atoi(line);
    fgets(line, MAX_LINE_SIZE, fr);
  }

  if(DBG) printf("Size:%d\n", n);

  *A = (double **) malloc(n * sizeof(double *));
  for(i=0;i<n;i++){
    (*A)[i] = (double *) malloc(n * sizeof(double));
  }

  if(DBG) printf("Initializing Matrix\n");

  for(i=0;i<n;i++){
    for(j=0;j<n;j++){
      (*A)[i][j] = 0.0;
    }
  }

  j=0;
  if(DBG) printf("Initialized Matrix\n");

  atmp = (double **) malloc(n * sizeof(double *));
  for(i=0;i<n;i++){
    atmp[i] = (double *) malloc(n * sizeof(double));
    if(fgets(line, 120, fr) != NULL) {
      tok = (char *) strtok(line, " ");
      while(tok) {
        tmp = atoi(tok);
        atmp[i][j] = (double)tmp;
        tok = (char *) strtok(NULL, " ");
        j++;
      }
      
      j=0;
    }
  }

  if(DBG) printf("File Read\n");

  fclose(fr);

  *A = atmp;

  return n;
}

void procedureLU(double **A, int n){
  int i, j, k;
  if(DBG) printf("Procedure Started\n");

  #pragma omp parallel private(k, i, j)
  {
    //#pragma omp for
    for(k=0;k<n;k++){
      #pragma omp for
      for(j=(k+1);j<n;j++){
        if(DBG) printf("In the j loop:A[%d][%d](%lf) with:%d from A[%d][%d](%lf) / A[%d][%d](%lf)\n", k, j, A[k][j] / A[k][k], omp_get_thread_num(), k, j, A[k][j], k, k, A[k][k]);
        A[k][j] = A[k][j] / A[k][k];
      }
      #pragma omp for
      for(i=(k+1);i<n;i++){
        for(j=(k+1);j<n;j++){
          if(DBG) printf("In the i loop:A[%d][%d](%lf) with:%d from A[%d][%d](%lf) - A[%d][%d](%lf) * A[%d][%d](%lf)\n", i, j, A[i][j] - (A[i][k] * A[k][j]), omp_get_thread_num(), i, j, A[i][j], i, k, A[i][k], k, j, A[k][j]);
          A[i][j] = A[i][j] - (A[i][k] * A[k][j]);
        }  
      }
    }
  }

  if(DBG) printf("Procedure Completed\n");

  printf("Parallel Algorithm\n");

  for(i=0;i<n;i++){
    for(j=0;j<n;j++){
      printf("%lf ", A[i][j]);
    }
    printf("\n");
  }
}

int main(int argc, char *argv[]){
  double **A;  
  int n;
  int i, j;
  n = 3;

  if(DBG) printf("Entering Main\n");

  n = readInputFile(&A);

  if(DBG) printf("Exiting ReadInputFile\n");

  procedureLU(A, n);

  if(DBG) printf("Program Done\n");

  return 0;
}
