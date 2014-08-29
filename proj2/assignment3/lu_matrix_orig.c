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

  for(i=0;i<(n);i++){
    for(j=(i+1);j<(n);j++){
      A[i][j] = A[i][j] / A[i][i];
      if(DBG) printf("in the j loop:A[%d][%d]\n", i, j);
    }
    for(j=(i+1);j<(n);j++){
      for(k=(i+1);k<(n);k++){
        A[j][k] = A[j][k] - (A[j][i]*A[i][k]);
        if(DBG) printf("in the i loop:A[%d][%d]\n", j, k);
      }
    }
  }

  if(DBG) printf("Procedure Completed\n");

  printf("Serial Algorithm\n");

  for(i=0;i<n;i++){
    for(j=0;j<n;j++){
      printf("%lf ", A[i][j]);
    }
    printf("\n");
  }
  printf("\n");
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
