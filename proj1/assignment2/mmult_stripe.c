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


int main (int argc, char* argv[])
{
  int rank, size, numtasks, source, dest, outbuf, i, tag=1, valid=0;
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
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  if(rank == 0) {
    printf("Read input file\n");
    size = readInputFile(&matrixA, &matrixB, &matrixC);
    printf("Size is %d\n", size);
  }

  if(pow(size, 2) == numtasks)
    valid = 1;

  if(valid) {
    dims[0] = size;
    dims[1] = size;

    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);
    MPI_Cart_coords(cartcomm, rank, 2, coords);
    MPI_Cart_shift(cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
    MPI_Cart_shift(cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);


    //outbuf = rank;

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

   //printf("READ INPUT FILE\n");
   fr = fopen("data.txt", "rt");
   while(fgets(line, 120, fr) != NULL) {
      //printf("LINE: '%s'\n", line);

      if(line[0] == '\0' || line[0] == '\n' || line[0] == '\r' || line[0] == '\t' || line[0] == ' ') {
         // Empty line separating the inputs (size/matrixA/matrixB)
         // IMPORTANT! This marks a state transition
         state++;
         i = 0;
         //printf("Read separator... New state is %d\n", state);
      } else if(state == 0) {
         //printf("Read size...\n");
         // Reading first line, containing the Size
         size = atoi(line);

         // Initialize the matrices
         initMatrix(&matrixA, size);
         initMatrix(&matrixB, size);
         initMatrix(&matrixC, size);
      } else if(state == 1) {
         //printf("Read Matrix A (size:%d), line %d...\n", (int)sizeof(matrixA), i);
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
         //printf("Read Matrix B, line %d...\n", i);
         // Reading matrix B
         // Initialize this row of the matrix
         matrixB[i] = (int *) malloc(size * sizeof(int));
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
      }
   }

   fclose(fr);

   *matrixAPtr = matrixA;
   *matrixBPtr = matrixB;
   *matrixCPtr = matrixC;

   return size;
}

/**
 * Initialize a square matrix
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
