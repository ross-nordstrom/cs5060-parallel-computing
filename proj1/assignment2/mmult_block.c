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
#define DBG    0

int readInputFile(int ***matrixAPtr, int ***matrixBPtr);
void initMatrix(int ***matrixPtr, int size);
void printMatrix(int **matrix, int size);


int **matrixA;
int **matrixB;
int **matrixC;

int main (int argc, char* argv[])
{
  int rank, size, numtasks, source, dest, outbuf, i, j, k, tag=1, valid=0, from, to, tmp;
  int dims[2]={4,4},
      periods[2]={0,0}, reorder=1, coords[2];
  int rankOffset, sendRank, recvRank, rtP;
  int **myMatrixA, **myMatrixB, **myMatrixC;
  int *workingA, *workingB;
  int *nbrsA, *nbrsB;
  char *tmpStr;
  FILE *fr;
  MPI_Status status;
  MPI_Request request;
  MPI_Datatype typeSubMatrix;  // We will create an MPI datatype to represent the sub-matrices

  /**
   * Setup the processes
   */
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  /**
   * Setup the matrix partition
   */
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);
  // Each process knows where he is relative to the other processes
  MPI_Cart_coords(cartcomm, rank, 2, coords);

  // Number of processes in each dimension (i.e. there are rtP processes in each row, and rtP in each column)
  rtP = sqrt(numtasks);

  nbrsA[coords[0]] = rank;
  nbrsB[coords[1]] = rank;
  for(i=1; i<rtP, i++) {
    MPI_Cart_shift(cartcomm, 0, i, &nbrsB[ (coords[1]-i) % rtP ], &nbrsB[ (coords[1]+i) % rtP ]);
    MPI_Cart_shift(cartcomm, 1, i, &nbrsA[ (coords[0]-i) % rtP ], &nbrsA[ (coords[0]+i) % rtP ]);
  }


  /**
   * Initialize matrix A and B, and the size (n)
   */
  if(rank == 0) {
    if(DBG) printf("Read input file\n");
    size = readInputFile(&matrixA, &matrixB);
    if(DBG) {
      printf("Size is %d\n", size);
      printf("Matrix A:\n");
      printMatrix(matrixA, size);
      printf("\nMatrix B:\n");
      printMatrix(matrixB, size);
      printf("\n\n");
      printf("NumTasks is %d\n", numtasks);
    }
  }

  /**
   * Validate inputs
   */
  if(rank==0 && size % sqrt(numtasks) != 0) {
    if(DBG) printf("ERROR: Expected size mod sqrt(numtasks) == 0\n");
    MPI_Finalize();
    return 1;
  }

  /**
   * Share the starting data, giving the appropriate data to each process
   *
   */
  if(rank == 0) {
    if(DBG) printf("Proc #%d sending size %d to %d processes...\n", rank, size, numtasks-1);

    // TODO - Use this instead of for loop
    // /**
    //  * Setup the datatype representing sub-matrices
    //  */
    // MPI_Type_vector( (size/rtP), (size/rtP), size, MPI_INT, &typeSubMatrix );

    for(i=1; i<numtasks; i++) {
      // Master process must send asynchronously
      MPI_Isend(&size, 1, MPI_INT, i, tag, MPI_COMM_WORLD, &request);
      MPI_Isend(matrixA[i], size, MPI_INT, i, 0, MPI_COMM_WORLD, &request);
      MPI_Isend(matrixB[i], size, MPI_INT, i, 0, MPI_COMM_WORLD, &request);

      myMatrixA = matrixA[0]; //(int *) malloc(sizeof(int) * size);
      myMatrixB = matrixB[0]; //(int *) malloc(sizeof(int) * size);
      myMatrixC = (int *) malloc(sizeof(int) * size);
    }
  } else {
    // printf("Proc #%d waiting for size...\n", rank);
    MPI_Recv(&size, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);

    /**
     * Setup the datatype representing sub-matrices
     */
    MPI_Type_vector( (size/rtP), (size/rtP), size, MPI_INT, &typeSubMatrix );

    /**
     * Each process has an array of size (n/rtP)*(n/rtP)
     */
    myMatrixA = (int **) malloc(sizeof(int *) * (n/rtP) );
    myMatrixB = (int **) malloc(sizeof(int *) * (n/rtP) );
    myMatrixC = (int **) malloc(sizeof(int *) * (n/rtP) );
    MPI_Recv(myMatrixA, size, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(myMatrixB, size, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    if(DBG) printf("Proc #%d got size: %d\n", rank, size);
  }

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
        MPI_Isend(&myMatrixB[j],        1, MPI_INT, sendRank, 1, MPI_COMM_WORLD, &request);
      }

      myMatrixC[j]=0;
      tmp = 0;
      for (k=0; k<size; k++) {
        if(k == rank) {
          tmp = myMatrixB[j];
        } else {
          MPI_Recv(&tmp, 1, MPI_INT, k, 1, MPI_COMM_WORLD, &status);
        }
        if(rank == 0 && DBG)   printf("%d. RCVD tmp <== %d, myMatrixC[%d] = %d\n", j,tmp, j, myMatrixC[j]);
        tmp = myMatrixA[k]*tmp;
        myMatrixC[j] += tmp;
      }
      MPI_Barrier(MPI_COMM_WORLD);
    } // for(j)
  } // for(i)
  if(DBG) printf("[%d] myMatrixC = [%d,%d,%d,%d]\n",rank, myMatrixC[0],myMatrixC[1],myMatrixC[2],myMatrixC[3]);

  if(rank == 0) {
    initMatrix(&matrixC, size);
    matrixC[0] = myMatrixC;
    for(k=1;k<size;k++) {
      // receive myMatrixC from process "k"
      if(DBG) printf("Collect myMatrixC from process %d\n", k);
      matrixC[k] = malloc(size*sizeof(int));
      MPI_Recv(matrixC[k], size, MPI_INT, k, 2, MPI_COMM_WORLD, &status);
      if(DBG) printf("Collected from %d\n", k);
    }
  } else {
    // send myMatrixC to root process
    if(DBG) printf("[%d] Return myMatrixC to root\n", rank);
    MPI_Send(myMatrixC, size, MPI_INT, 0, 2, MPI_COMM_WORLD);
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
  MPI_Finalize();
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
   int state = 0, i, j, size = 0, tmp;
   int **matrixA, **matrixB, **matrixC;
   FILE *fr;
   char line[120];
   char *tok;

   fr = fopen("data.txt", "rt");
   while(fgets(line, 120, fr) != NULL) {

      if(line[0] == '\0' || line[0] == '\n' || line[0] == '\r' || line[0] == '\t' || line[0] == ' ') {
         // Empty line separating the inputs (size/matrixA/matrixB)
         // IMPORTANT! This marks a state transition
         state++;
         i = 0;
      } else if(state == 0) {
         // Reading first line, containing the Size
         size = atoi(line);

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
