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
#include <math.h>
#define SIZE   16
#define UP     0
#define DOWN   1
#define LEFT   2
#define RIGHT  3
 
int main (int argc, char* argv[])
{
  int rank, size, numtasks, source, dest, outbuf, i, tag=1, valid=0;
  int inbuf[4]={MPI_PROC_NULL,MPI_PROC_NULL,MPI_PROC_NULL,MPI_PROC_NULL},
      nbrs[4], dims[2]={4,4},
      periods[2]={0,0}, reorder=0, coords[2];

  MPI_Request reqs[8];
  MPI_Status stats[8];
  MPI_Comm cartcomm;

  //printf("Rank, Xpos, Ypos,  Type, UP, DOWN, LEFT, RIGHT\n");
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  
  size = sqrt(numtasks);
  if(pow(size, 2) == numtasks)
    valid = 1;
  //printf("SQRT = %d, VALID = %s\n", size, (valid ? "YES" : "NO"));

  if(valid) {
    dims[0] = size;
    dims[1] = size;

    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);
    MPI_Comm_rank(cartcomm, &rank);
    MPI_Cart_coords(cartcomm, rank, 2, coords);
    MPI_Cart_shift(cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
    MPI_Cart_shift(cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);

    outbuf = rank;

    for(i=0; i<4; i++) {
      dest = nbrs[i];
      source = nbrs[i];
      MPI_Isend(&outbuf, 1, MPI_INT, dest, tag,
                  MPI_COMM_WORLD, &reqs[i]);
      MPI_Irecv(&inbuf[i], 1, MPI_INT, source, tag,
                  MPI_COMM_WORLD, &reqs[i+4]);
    }

    MPI_Waitall(8, reqs, stats);

    printf("%d/%s (%d,%d)\n",
            rank, "POS", coords[0], coords[1]);
    printf("%d/%s (%d,%d,%d,%d)\n",
            rank, "NBRS", nbrs[UP], nbrs[DOWN], nbrs[LEFT], nbrs[RIGHT]);
    printf("%d/%s (%d,%d,%d,%d)\n",
            rank, "INBUF", inbuf[UP], inbuf[DOWN], inbuf[LEFT], inbuf[RIGHT]);
  } else {
    printf("Must specify a power-of-2 number of processors. Terminating.\n");
  }

  MPI_Finalize();
  return 0;
}
