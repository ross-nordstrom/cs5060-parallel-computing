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
#define DBG2    1

int readInputFile(int ***matrixAPtr, int ***matrixBPtr);
void initMatrix(int ***matrixPtr, int size);
void printMatrix(int **matrix, int size);


int **matrixA;
int **matrixB;

int main (int argc, char* argv[])
{
  int rank, size, numtasks, source, dest, outbuf, i, j, k, l, tag=1, valid=0, from, to, tmp;
  int dims[2],
      periods[2]={0,0}, reorder=1, coords[2];
  int rankOffset, sendRank, recvRank, rtP;
  int **matrixC;
  int **myMatrixA, **myMatrixB, **myMatrixC;
  int *workingA, *workingB, *myBColumn;
  int *nbrsA, *nbrsB;
  int index1, index2, nbr1, nbr2, myN;
  char *tmpStr;
  FILE *fr;
  MPI_Status status;
  MPI_Request request;
  MPI_Comm cartcomm;
  MPI_Datatype typeSubMatrix;  // We will create an MPI datatype to represent the sub-matrices


  /**
   * Initialize matrix A and B, and the size (n)
   */
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

  /**
   * Setup the processes
   */
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  // Number of processes in each dimension (i.e. there are rtP processes in each row, and rtP in each column)
  rtP = (int) sqrt(numtasks);
  dims[0] = rtP;
  dims[1] = rtP;
  myN = size / rtP;
  nbrsA = malloc( sizeof(int) * rtP);
  nbrsB = malloc( sizeof(int) * rtP);

  if(DBG) printf("[%d] Dims: [%d,%d]\n", rank, dims[0], dims[1]);

  /**
   * Validate inputs
   */
  if(size % rtP != 0) {
    if(rank==0) printf("ERROR: Expected size mod sqrt(numtasks) == 0\n");
    MPI_Finalize();
    return 1;
  }

  /**
   * Setup the matrix partition
   */
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);
  // Each process knows where he is relative to the other processes
  MPI_Cart_coords(cartcomm, rank, 2, coords);

  if(DBG) printf("[%d] Coords: (%d,%d)\n", rank, coords[0], coords[1]);

  nbrsA[coords[1]] = rank;
  nbrsB[coords[0]] = rank;

  /**
   * Figure out who my vertical and horizontal neighbors are, using cartesian communication
   */
  for(i=1; i<rtP; i++) {
    // My Y-pos - StepSize, mo
    index1 = (coords[0]-i); // Up
    index2 = (coords[0]+i);

    if(DBG) printf("[%d] (%d) U/D --  i1: %d, i2: %d\n", rank, i, index1, index2);

    MPI_Cart_shift(cartcomm, 0, i, &nbr1, &nbr2);

    if(DBG) printf("[%d] (%d) U/D --  n1: %d, n2: %d\n", rank, i, nbr1, nbr2);
    if(index1 >= 0 && index1 < rtP) nbrsB[index1] = nbr1;
    if(index2 >= 0 && index1 < rtP) nbrsB[index2] = nbr2;

    index1 = coords[1] - i;
    index2 = coords[1] + i;
    if(DBG) printf("[%d] (%d) L/R --  i1: %d, i2: %d\n", rank, i, index1, index2);

    MPI_Cart_shift(cartcomm, 1, i,  &nbr1, &nbr2);

    if(DBG) printf("[%d] (%d) L/R --  n1: %d, n2: %d\n", rank, i, nbr1, nbr2);
    if(index1 >= 0 && index1 < rtP) nbrsA[index1] = nbr1;
    if(index2 >= 0 && index1 < rtP) nbrsA[index2] = nbr2;
  }

  /**
   * Wait to make sure everyone knows their neighbors
   */
  MPI_Barrier(MPI_COMM_WORLD);
  if(DBG) printf("[%d] nbrsA: [%d,%d] nbrsB: [%d,%d]\n", rank, nbrsA[0], nbrsA[1], nbrsB[0], nbrsB[1]);

  /**
   * Initialize the starting data
   */
  myMatrixA = (int **) malloc(sizeof(int *) * myN);
  myMatrixB = (int **) malloc(sizeof(int *) * myN);
  myMatrixC = (int **) malloc(sizeof(int *) * myN);
  workingA = (int *) malloc(sizeof(int) * myN);
  workingB = (int *) malloc(sizeof(int) * myN);
  myBColumn = (int *) malloc(sizeof(int) * myN);

  for(i=0; i<myN; i++) {
    // Initliaze this row of each of my matrices
    myMatrixA[i] = (int *) malloc(sizeof(int) * myN);
    myMatrixB[i] = (int *) malloc(sizeof(int) * myN);
    myMatrixC[i] = (int *) malloc(sizeof(int) * myN);

    if(rank==0){
      //use myMatrixA/B as temp
      for(j=1; j<numtasks; j++){ 
        index1 = j / rtP; // Get the row this processor is in
        index2 = j % rtP; // Get the column this processor is in

        for(k=0; k<myN; k++) {
          myMatrixA[i][k] = matrixA[index1*myN+i][index2*myN+k];
          myMatrixB[i][k] = matrixB[index1*myN+i][index2*myN+k];
        }

        MPI_Isend(myMatrixA[i], myN, MPI_INT, j, i, MPI_COMM_WORLD, &request);
        MPI_Isend(myMatrixB[i], myN, MPI_INT, j, i, MPI_COMM_WORLD, &request);
      }

      index1 = rank / rtP;
      index2 = rank % rtP;
      
      for(j=0; j<myN; j++) {
        myMatrixA[i][j] = matrixA[index1*myN+i][index2*myN+j];
        myMatrixB[i][j] = matrixB[index1*myN+i][index2*myN+j];
      }
    }else{
      MPI_Recv(myMatrixA[i], myN, MPI_INT, 0, i, MPI_COMM_WORLD, &status);
      MPI_Recv(myMatrixB[i], myN, MPI_INT, 0, i, MPI_COMM_WORLD, &status);
    }
    /*for(j=0; j<myN; j++) {
      myMatrixA[i][j] = matrixA[index1*myN+i][index2*myN+j];
      myMatrixB[i][j] = matrixB[index1*myN+i][index2*myN+j];
    } */ // iterate over each column
  } // iterate over each row

  // TODO: Clear matrixA and matrixB from mem
  MPI_Barrier(MPI_COMM_WORLD);
  if(DBG2) {
    // Verify correct assignment
    for(i=0; i<numtasks; i++) {
      if(rank==i) {
        if(DBG) printf("[%d] MY MATRIX A: \n", rank);
        if(DBG) printMatrix(myMatrixA, myN);
        if(DBG) printf("[%d] MY MATRIX B: \n", rank);
        if(DBG) printMatrix(myMatrixB, myN);
        if(DBG) printf("\n");
      }
      MPI_Barrier(MPI_COMM_WORLD);
    }
  } // DBG

  // Because we have to manually setup each column, operate column by column for better efficiency
  for(j=0; j<myN; j++) {
    // Setup my matrix B column
    for(i=0; i<myN; i++) {
      myBColumn[i] = myMatrixB[i][j];
    }
    if(DBG && rank==0) printf("[%d] (%d) My B Column: [%d,%d]\n", rank, j, myBColumn[0], myBColumn[1]);
    // Now, iterate over each of my rows
    for(i=0; i<myN; i++) {
      /**
       * Share my data with all my neighbors
       *
       * Send my row to all of my horizontal neighbors, and receive each of their rows
       * Send my column to all of my vertical neighbors, and receive each of their columns
       */
      for(k=0; k<rtP; k++) {
        if(nbrsA[k] != rank) {
          // Tag with my negative rank to mark it as MY MATRIX A
          if(DBG && rank==0) printf("[%d] (%d,%d) Send myMatrixA[%d] to %d\n",rank,i,j,i,nbrsA[k]);
          MPI_Isend(myMatrixA[i], myN, MPI_INT, nbrsA[k], 1, MPI_COMM_WORLD, &request);
        }
        if(nbrsB[k] != rank) {
          // Tag with my positive rank to mark it as MY MATRIX B
          if(DBG && rank==0) printf("[%d] (%d,%d) Send myBColumn to %d\n",rank,i,j,nbrsB[k]);
          MPI_Isend(myBColumn, myN, MPI_INT, nbrsB[k], 2, MPI_COMM_WORLD, &request);
        }
      } // for each neighbor
      // Synchronize to ensure all messages have been sent
      MPI_Barrier(MPI_COMM_WORLD);
      /**
       * Calculate this C element, receiving data as needed from neighbors
       */
      myMatrixC[i][j] = 0;
      for(k=0; k<rtP; k++) {
        if(DBG && rank==0) printf("[%d] (%d,%d) Nbr:%d, VertNbrRank:%d, HorizNbrRank:%d\n",rank,i,j,k,nbrsA[k],nbrsB[k]);
        if(nbrsA[k] != rank) {
          // Receive row of A from this neighbor
          if(DBG && rank==0) printf("[%d] (%d,%d) Get workingA from %d\n",rank,i,j,nbrsA[k]);
          MPI_Recv(workingA, myN, MPI_INT, nbrsA[k], 1, MPI_COMM_WORLD, &status);
        } else {
          memcpy(workingA, myMatrixA[i], myN*sizeof(int));
        }
        if(nbrsB[k] != rank) {
          // Receive column of B from this neighbor
          if(DBG && rank==0) printf("[%d] (%d,%d) Get workingB from %d\n",rank,i,j,nbrsB[k]);
          MPI_Recv(workingB, myN, MPI_INT, nbrsB[k], 2, MPI_COMM_WORLD, &status);
        } else {
          memcpy(workingB, myBColumn, myN*sizeof(int));
        }

        // Calculate with these two neighbors' data
        if(DBG && rank==0) printf("[%d] (%d,%d) Calculate: workingA = [%d,%d]\n",rank,i,j,workingA[0],workingA[1]);
        if(DBG && rank==0) printf("[%d] (%d,%d) Calculate: workingB = [%d,%d]\n",rank,i,j,workingB[0],workingB[1]);
        for(l=0; l<myN; l++) {
          tmp = workingA[l]*workingB[l];
          myMatrixC[i][j] += tmp;
        }
        if(DBG && rank==0) printf("[%d] (%d,%d) Calculated: myMatrixC[*][%d] = [%d,%d]\n",rank,i,j,j,myMatrixC[0][j],myMatrixC[1][j]);

      } // for each neighbor

      // Synchronize on each cell
      MPI_Barrier(MPI_COMM_WORLD);

    } // for each of my rows
  } // for each of my columns

  MPI_Barrier(MPI_COMM_WORLD);
  if(DBG) {
    // Verify correct assignment
    for(i=0; i<numtasks; i++) {
      if(rank==i) {
        if(DBG) printf("[%d] MY MATRIX C: \n", rank);
        if(DBG) printMatrix(myMatrixC, myN);
        if(DBG) printf("\n");
      }
      MPI_Barrier(MPI_COMM_WORLD);
    }
  } // DBG

  /**
   * Consolidate the results onto master process
   */
  if(rank != 0) {
    for(i=0; i<myN; i++) {
      // Tag it with which row this is
      MPI_Isend(myMatrixC[i], myN, MPI_INT, 0, i, MPI_COMM_WORLD, &request);
    } // for each of my rows
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if(rank == 0) {
    if(DBG) printf("Initialize matrixC...\n");
    initMatrix(&matrixC, size);
    if(DBG) printf("Initialized matrixC.\n");

    for(k=0;k<numtasks;k++) {
      if(DBG) printf("(PROC:%d) Receive theirMatrixC\n",k);
      index1 = k / rtP; // Get the row this processor is in
      index2 = k % rtP; // Get the column this processor is in
      for(i=0; i<myN; i++) {
        if(DBG) printf("(PROC:%d, %d) Receive a row\n",k, i);
        if(k == 0) {
          if(DBG) printf("(PROC:%d, %d) myMatrixC[%d] = [%d,%d]\n",k, i, i, myMatrixC[i][0], myMatrixC[i][1]);
          workingA = myMatrixC[i];
        } else {
          MPI_Recv(workingA, myN, MPI_INT, k, i, MPI_COMM_WORLD, &status);
          if(DBG) printf("(PROC:%d, %d) workingC[%d]=[%d,%d]\n",k, i, i, workingA[0], workingA[1]);
          // for(j=0;j<myN;j++) matrixC[index1*myN + i][index2*myN + j] = workingA[j];
        }
        memcpy( (matrixC[index1*myN+i] + index2*myN), workingA, myN*sizeof(int));
        // memcpy(matrixC[index1*myN+i] + index2*myN), workingA, myN*sizeof(int));
        if(DBG) printf("(PROC:%d, %d) matrixC[%d]=[%d,%d]\n",k, i, i, matrixC[index1*myN + i][0], matrixC[index1*myN + i][1]);
      } // for each of their rows
    } // for each slave process

  }


  if(rank == 0) {
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
   int state = 0, i, j, size = 0, tmp;
   int **matrixA, **matrixB, **matrixC;
   int maxStrSize = 120;
   FILE *fr;
   char *line = malloc(maxStrSize*sizeof(char));
   char *tok;

   fr = fopen("data.txt", "rt");
   while(fgets(line, maxStrSize, fr) != NULL) {
      if(line[0] == '\0' || line[0] == '\n' || line[0] == '\r' || line[0] == '\t' || line[0] == ' ') {
         // Empty line separating the inputs (size/matrixA/matrixB)
         // IMPORTANT! This marks a state transition
         state++;
         i = 0;
      } else if(state == 0) {
         // Reading first line, containing the Size
         size = atoi(line);

         // Initialize the line string, assuming worst case 6-char per specified matrix cell
         maxStrSize = size*6;
         line = malloc(maxStrSize*sizeof(char));

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
