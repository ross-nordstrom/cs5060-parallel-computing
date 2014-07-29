/********************************************************************************
 * Cartesian Topology
 *
 * Use MPI functions for performing coordinate-to-rand and rank-to-coordinate
 * translations.
 *
 *  int MPI_Cart_rank(MPI_Comm comm_cart, int *coords, int *rank)
 *  int MPI_Cart_coords(MPI_Comm comm_cart, int rank, int maxdims, int *coords)
 *
 * Compute the rank of the src and dst in a shift:
 *
 *  int MPI_Cart_shift(MPI_Comm comm_cart, int dir, int s_step
 *  int *rank_source, int *rank_dest)
 *
 ***/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define UP     0
#define DOWN   1
#define LEFT   2
#define RIGHT  3
#define DBG    1

int readInputFile(int ***matrixAPtr, int ***matrixBPtr);
void initMatrix(int ***matrixPtr, int size);
void printMatrix(int **matrix, int size);


int **matrixA;
int **matrixB;
int **matrixC;

int main (int argc, char* argv[])
{
  int rank, myN, size, numtasks, source, dest, outbuf, i, j, k, tag=1, valid=0, from, to, tmp;
  int inbuf[4]={MPI_PROC_NULL,MPI_PROC_NULL,MPI_PROC_NULL,MPI_PROC_NULL},
      nbrs[4], dims[2]={4,4},
      periods[2]={1,1}, reorder=1, coords[2];
  int rankOffset, sendRank, recvRank, sendTag, recvTag;
  int **myMatrixA, **myMatrixB, **myMatrixC;
  char *tmpStr;
  FILE *fr;
  MPI_Status status;
  MPI_Request request;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if(rank == 0) {
    if(DBG) printf("Read input file\n");
    size = readInputFile(&matrixA, &matrixB);
    myN = size/numtasks;   // n/P
    if(DBG) {
      printf("Size is %d\n", size);
      printf("Matrix A:\n");
      printMatrix(matrixA, size);
      printf("\nMatrix B:\n");
      printMatrix(matrixB, size);
      printf("\n\n");
      printf("NumTasks is %d\n", numtasks);
    }

    if(size % numtasks != 0) {
      if(rank==0 && DBG) printf("ERROR: Expected size mod numtasks == 0\n");
      MPI_Finalize();
      return 1;
    }
  }

  // Share the data
  if(rank == 0) {
    if(DBG) printf("Proc #%d sending size %d to %d processes...\n", rank, size, numtasks-1);
    for(i=1; i<numtasks; i++) {
      MPI_Isend(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &request);
    }

    if(DBG) printf("Proc #%d sending data of size %dx%d to %d processes...\n", rank, size, myN, numtasks-1);
    for(i=myN; i<size; i++) {
      sendRank = i / myN;
      sendTag = i % myN;
      // Master process must send asynchronously
      if(DBG) printf("[%d] Send row %d to %d with tag %d\n", rank, i, sendRank, sendTag);
      MPI_Isend(matrixA[i], size, MPI_INT, sendRank, sendTag, MPI_COMM_WORLD, &request);
      MPI_Isend(matrixB[i], size, MPI_INT, sendRank, sendTag, MPI_COMM_WORLD, &request);
    } 

    if(DBG) printf("All Sent.\n");

    for(i=0; i<myN; i++){
      // initialize this row
      if(DBG) printf("[%d] Malloc my row %d\n", rank, i);
      myMatrixA[i] = (int *) malloc(sizeof(int) * size);
      myMatrixB[i] = (int *) malloc(sizeof(int) * size);
      myMatrixC[i] = (int *) malloc(sizeof(int) * size);

      if(DBG) printf("[%d] Initialize my row %d\n", rank, i);
      for(j=0; j<size; j++) {
        myMatrixA[i][j] = matrixA[i][j]; //(int *) malloc(sizeof(int) * size);
        myMatrixB[i][j] = matrixB[i][j]; //(int *) malloc(sizeof(int) * size);
      }
    }
  } else {
    if(DBG) printf("Proc #%d waiting for size from master...\n", rank);
    MPI_Recv(&size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    myN = size/numtasks;
    if(DBG) printf("Proc #%d got data from master: size=%d, myN=%d\n", rank, size, myN);
      myMatrixA = (int **) malloc(sizeof(int *) * size);
      myMatrixB = (int **) malloc(sizeof(int *) * size);
      myMatrixC = (int **) malloc(sizeof(int *) * size);
    if(DBG) printf("Proc #%d waiting for %d rows of width %d from master...\n", rank, myN, size);
    for(i=0; i<myN; i++){
      // initialize this row
      if(DBG) printf("[%d] Malloc my row %d\n", rank, i);
      myMatrixA[i] = (int *) malloc(sizeof(int) * size);
      myMatrixB[i] = (int *) malloc(sizeof(int) * size);
      myMatrixC[i] = (int *) malloc(sizeof(int) * size);

      if(DBG) printf("[%d] Recv row %d...\n", rank, i);
      MPI_Recv(myMatrixA[i], size, MPI_INT, 0, i, MPI_COMM_WORLD, &status);
      MPI_Recv(myMatrixB[i], size, MPI_INT, 0, i, MPI_COMM_WORLD, &status);
      if(DBG) printf("Proc #%d got row %d\n", rank, i);
    }
  }

  if(DBG) printf("Proc #%d at post-setup barrier\n", rank);
  MPI_Barrier(MPI_COMM_WORLD);

  // Assume each process just has a single row
  from = rank * size/numtasks;
  to = (rank+1) * size/numtasks;

  // Each process's job loop -- should just have 1 row per process
  for (i=from; i<to; i++) {

    // Loop over each element in the row (of matrixA)
    for (j=0; j<size; j++) {

      // Asynchronously send my value to everyone else
      for(k=1; k < size; k++) {
        sendRank = (rank + k) % size;
        // if(sendRank == 0 )
        //   if(DBG)  printf("(%d)   [%d] Send %d to [%d]\n", j, rank, myMatrixB[j], sendRank);
        MPI_Isend(&myMatrixB[i][j],        1, MPI_INT, sendRank, 1, MPI_COMM_WORLD, &request);
      }

      myMatrixC[i][j]=0;
      tmp = 0;
      for (k=0; k<size; k++) {
        if(k == rank) {
          tmp = myMatrixB[i][j];
        } else {
          MPI_Recv(&tmp, 1, MPI_INT, k, 1, MPI_COMM_WORLD, &status);
        }
        if(rank == 0 && DBG)   printf("%d. RCVD tmp <== %d, myMatrixC[%d] = %d\n", j,tmp, j, myMatrixC[i][j]);
        tmp = myMatrixA[i][k]*tmp;
        myMatrixC[i][j] += tmp;
      }
      MPI_Barrier(MPI_COMM_WORLD);
    } // for(j)
  } // for(i)
  if(DBG) printf("[%d] myMatrixC = [%d,%d,%d,%d]\n",rank, myMatrixC[0][0],myMatrixC[0][1],myMatrixC[0][2],myMatrixC[0][3]);

  if(rank == 0) {
    initMatrix(&matrixC, size);
    for(k=0; k<(size/numtasks); k++){
      matrixC[k] = myMatrixC[k];
    }
    for(k=(size/numtasks); k<size; k++) {
      // receive myMatrixC from process "k"
      if(DBG) printf("Collect myMatrixC from process %d\n", k);
      matrixC[k] = malloc(size*sizeof(int));
      recvRank = k / numtasks;
      MPI_Recv(matrixC[k], size, MPI_INT, recvRank, 2, MPI_COMM_WORLD, &status);
      if(DBG) printf("Collected from %d\n", k);
    }
  } else {
    // send myMatrixC to root process
    if(DBG) printf("[%d] Return myMatrixC to root\n", rank);
    for(k=0; k<(size/numtasks); k++){
      MPI_Send(myMatrixC[k], size, MPI_INT, 0, 2, MPI_COMM_WORLD);
    }
  }

  if(DBG) printf("[%d] Done.\n", rank);
  MPI_Barrier(MPI_COMM_WORLD);

  if(rank == 0) {
    // printf("DONE!\n");

    if(DBG) {
      printf("\n\n");
      printMatrix(matrixA, size);
      printf("\n\t       * \n");
      printMatrix(matrixB, size);
      printf("\n\t       = \n");
    }
    printMatrix(matrixC, size);
  }

  MPI_Barrier(MPI_COMM_WORLD);
  if(rank == 0) MPI_Finalize();
  return 0;
}


/**
* Have Process 0 read the file input "data.txt", containing:
*   <Size (n)>
*   A A ... A
*   A A ... A
*      ...
*   A A ... A
*
*   B B ... B
*   B B ... B
*      ...
*   B B ... B
*/
int readInputFile(int ***matrixAPtr, int ***matrixBPtr) {
   /**
    * state:   0 - read size
    *          1 - read matrix a
    *          2 - read matrix b
    * i:       Marks which row within matrix A or B we are at
    *             (corresponding to a Row in the input txt file)
    * j:       Marks which column within the matrix row we are at
    */
   int state = 0, i, j, size = 120, tmp;
   int **matrixA, **matrixB, **matrixC;
   FILE *fr;
   char *line = malloc(size*6*sizeof(char));
   char *tok;

   fr = fopen("data.txt", "rt");
   while(fgets(line, size*6, fr) != NULL) {
      if(line[0] == '\0' || line[0] == '\n' || line[0] == '\r' || line[0] == '\t' || line[0] == ' ') {
         // Empty line separating the inputs (size/matrixA/matrixB)
         // IMPORTANT! This marks a state transition
         state++;
         i = 0;
      } else if(state == 0) {
         // Reading first line, containing the Size
         size = atoi(line);

         // Initialize the line string, assuming worst case 6-char per specified matrix cell
         line = malloc(size*6*sizeof(char));

         // Initialize the matrices
         initMatrix(&matrixA, size);
         initMatrix(&matrixB, size);
         initMatrix(&matrixC, size);
      } else if(state == 1) {
         // Reading matrix A
         // Initialize this row of the matrix
         j = 0;
         // Read the row into the matrix
         tok = (char *) strtok(line, " ");
         while(tok) {
            tmp = atoi(tok);
            matrixA[i][j] = tmp;
            tok = (char *) strtok(NULL, " ");
            j++;
         }
         i++;
      } else if(state == 2) {
         // Reading matrix B
         // Initialize this row of the matrix
         matrixB[i] = (int *) malloc(size * sizeof(int));
         j = 0;
         // Read the row into the matrix
         tok = (char *) strtok(line, " ");
         while(tok) {
            tmp = atoi(tok);
            matrixB[i][j] = tmp;
            tok = (char *) strtok(NULL, " ");
            j++;
         }
         i++;
      }
   }

   fclose(fr);

   *matrixAPtr = matrixA;
   *matrixBPtr = matrixB;

   return size;
}

/**
 * Initialize a square matrix of size "size*size"
 */
void initMatrix(int ***matrixPtr, int size) {
   int i;
   int **matrix;

   matrix = (int **) malloc(sizeof(int *) * size);
   for(i=0; i < size; i++) {
      matrix[i] = (int *) malloc(sizeof(int) * size);
   }
   *matrixPtr = matrix;
}

void printMatrix(int **matrix, int size) {
   int i, j;
   for(i=0; i<size; i++) {
      printf("  ");
      for(j=0; j<size; j++) {
         printf("%3d ", matrix[i][j]);
      }
      printf("\n");
   }
}
