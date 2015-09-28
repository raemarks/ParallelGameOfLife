#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>


#include "gol.h"


int main(int argc, char *argv[])
{
	struct timeval t_total1, t_total2;

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

	/* I'm assuming this isn't counted in the total runtime. */
	GenerateInitialGoL();

	gettimeofday(&t_total1, NULL);
	Simulate();
	gettimeofday(&t_total2, NULL);
	/* Get time in milliseconds to call MPI_Bcast on average */
	total_runtime = (t_total2.tv_sec-t_total1.tv_sec)*1000 +
	    ((double) t_total2.tv_usec-t_total1.tv_usec)/1000;

	/* Not going to count this in the timing, since it's just to sync
	 * before we print at the end */
	MPI_Barrier(MPI_COMM_WORLD);
	sleep(1);

	printf("%d: Total runtime for simulation on %d generations: %lfms\n",
	    rank, generations, total_runtime - display_time);
	printf("%d: Average time per generation: %lfms\n", rank,
	    (total_runtime - display_time)/generations);
	printf("%d: Average time per display function: %lfms\n", rank,
	    display_time/displays);
	printf("%d: Average time per MPI_Barrier call: %lfms\n", rank,
	    barrier_time/barriers);
	printf("%d: Average time per MPI_Send call: %lfms\n", rank,
	    send_time/sends);
	printf("%d: Average time per MPI_Recv call: %lfms\n", rank,
	    recv_time/recvs);

	MPI_Finalize();

	return 0;
}
