#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>

#include "gol.h"


int main(int argc, char *argv[])
{
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&p);

	assert(p>=2);

	if (rank == 0)
		printf("p: %d\n", p);

	if (rank == 0) {
		printf("Choose an integer n for the n X n grid: \n");
		scanf("%d", &n);
		printf("Number of generations: \n");
		scanf("%d", &generations);
	}
	/* Send n and generations to all procs */
	MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&generations, 1, MPI_INT, 0, MPI_COMM_WORLD);

	GenerateInitialGoL();
	Simulate();

	MPI_Finalize();

	return 0;
}
