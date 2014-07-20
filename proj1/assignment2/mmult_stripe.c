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
#define SIZE   16
#define UP     0
#define DOWN   1
#define LEFT   2
#define RIGHT  3

int readInputFile(int ***matrixAPtr, int ***matrixBPtr, int ***matrixCPtr);
void initMatrix(int ***matrixPtr, int size);
void printMatrix(int **matrix, int size);

int main (int argc, char* argv[])
{
  int rank, size, numtasks, source, dest, outbuf, i, j, k, tag=1, valid=0, from, to;
  int inbuf[4]={MPI_PROC_NULL,MPI_PROC_NULL,MPI_PROC_NULL,MPI_PROC_NULL},
      nbrs[4], dims[2]={4,4},
      periods[2]={1,1}, reorder=1, coords[2];
  int **matrixA;
  int **matrixB;
  int **matrixC;
  FILE *fr;

  MPI_Request reqs[8];
  MPI_Status stats[8];
  MPI_Comm cartcomm;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  
  if(rank == 0) {
    printf("Read input file\n");
    size = readInputFile(&matrixA, &matrixB, &matrixC);
    printf("Size is %d\n", size);
    printf("Matrix A:\n");
    printMatrix(matrixA, size);
    printf("\nMatrix B:\n");
    printMatrix(matrixB, size);
    printf("\n\n");
    printf("NumTasks is %d\n", numtasks);
  }

  if(pow(size, 2) == numtasks)
    valid = 1;

  if(valid) {
   MPI_Bcast(matrixB, size*size, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Scatter(matrixA, size*size/numtasks, MPI_INT, matrixA[from], size*size/numtasks, MPI_INT, 0, MPI_COMM_WORLD);

   from = rank * size/numtasks;
   to = (rank+1) * size/numtasks;

   printf("[%d] from:%d, to:%d\n",rank,from,to);

   printf("computing slice %d (from row %d to %d)\n", rank, from, to-1);
   for (i=from; i<to; i++) {
      for (j=0; j<SIZE; j++) {
        matrixC[i][j]=0;
        for (k=0; k<SIZE; k++) {
            matrixC[i][j] += matrixA[i][k]*matrixB[k][j];
        }
      }
   }
  
    MPI_Gather (matrixC[from], size*size/numtasks, MPI_INT, matrixC, size*size/numtasks, MPI_INT, 0, MPI_COMM_WORLD);
  
    if (rank==0) {
      printf("DONE!\n");
      printf("\n\n");
      printMatrix(matrixA, size);
      printf("\n\n\t       * \n");
      printMatrix(matrixB, size);
      printf("\n\n\t       = \n");
      printMatrix(matrixC, size);
      printf("\n\n");
    }


//    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);
//    MPI_Cart_coords(cartcomm, rank, 2, coords);
//    MPI_Cart_shift(cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
//    MPI_Cart_shift(cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);


//     for(i=0; i<4; i++) {
//       dest = nbrs[i];
//       source = nbrs[i];
//       MPI_Isend(&outbuf, 1, MPI_INT, dest, tag,
//                   MPI_COMM_WORLD, &reqs[i]);
//       MPI_Irecv(&inbuf[i], 1, MPI_INT, source, tag,
//                   MPI_COMM_WORLD, &reqs[i+4]);
//     }
// 
//     MPI_Waitall(8, reqs, stats);
// 
//     printf("%d/%s (%d,%d)\n",
//             rank, "POS", coords[0], coords[1]);
//     printf("%d/%s (%d,%d,%d,%d)\n",
//             rank, "NBRS", nbrs[UP], nbrs[DOWN], nbrs[LEFT], nbrs[RIGHT]);
//     printf("%d/%s (%d,%d,%d,%d)\n",
//             rank, "INBUF", inbuf[UP], inbuf[DOWN], inbuf[LEFT], inbuf[RIGHT]);

  } else {
    if(rank == 0) {
      printf("Must specify a power-of-2 number of processors. Terminating.\n");
    }
  }

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
int readInputFile(int ***matrixAPtr, int ***matrixBPtr, int ***matrixCPtr) {
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
   *matrixCPtr = matrixC;

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
