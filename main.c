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
	double comm_time, comp_time;

	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&p);

	assert(p>=1);
	assert(argc >= 3);

	n = atoi(argv[1]);
	generations = atoi(argv[2]);

	if (rank == 0)
		printf("p: %d\n", p);

#if 0
	if (rank == 0) {
		printf("Choose an integer n for the n X n grid: \n");
		scanf("%d", &n);
		printf("Number of generations: \n");
		scanf("%d", &generations);
	}
#endif
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


	/* Moving on to reporting stage. Will report in a csv format which can
	 * be copied straight into a spreadsheet application for analysis. */


	/* Not going to count this in the timing, since it's just to sync
	 * before we print at the end */
	MPI_Barrier(MPI_COMM_WORLD);
	sleep(1);
	if (rank == 0) {
		/* Print rank, p, n, total time, time per generation, communication
		 * time per generation, compute time per generation, display
		 * time per generation, communicaiton time per display */
		printf("rank, p, n, total, time/gen, comm/gen, comp/gen, display/gen,"
		    " comm/display, sendrecv/gen, barrier/gen\n");
	}
	/* Not going to count this in the timing, since it's just to sync
	 * before we print at the end */
	MPI_Barrier(MPI_COMM_WORLD);
	sleep(1);

	/* Make sure total runtime excludes the display time. */
	total_runtime -= display_time;
	comm_time = sendrecv_time+barrier_time;
	comp_time = total_runtime - comm_time;

	printf("%d, %d, %d, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf\n",
	    rank,
	    p,
	    n,
	    total_runtime,
	    total_runtime/generations,
	    comm_time/generations,
	    comp_time/generations,
	    display_time/generations,
	    gather_time/displays,
	    sendrecv_time/generations,
	    barrier_time/generations
	    );

	MPI_Finalize();

	return 0;
}
