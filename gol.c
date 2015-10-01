#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <time.h>
#include <sys/time.h>

#include "gol.h"

#define BIG_PRIME 982451653

#define MAX_N 8192

/* MPI variables */
int rank;
int p;
/* Cells per col/row in entire grid */
int n;
/* Rows of grid managed by this proc */
int nrows;
/* chunk of grid this proc controls, plus placeholder rows for border rows from
 * other procs. i.e. this chunk plus two rows. Also need next_grid to swap
 * between generations. */
char *grid;
char *next_grid;
/* Used for MPI_Gatherv when gathering whole grid to print */
char *display_grid;
int *recv_counts;
int *displs_counts;
/* rows containing cells above the top row and below the bottom row of this
 * chunk */
char *above_neighbors;
char *below_neighbors;
/* Top and bottom rows of chunk controlled by this proc */
char *top_row;
char *bottom_row;
/* Procs to talk to to get top and bottom neighbor cells */
int above_rank;
int below_rank;
/* Number of generations for execution */
int generations;

double total_runtime = 0;
double display_time = 0;
int displays = 0;
double send_time = 0;
int sends = 0;
double recv_time = 0;
int recvs = 0;
double barrier_time = 0;
int barriers = 0;
double bcast_time = 0;
int bcasts = 0;
double gather_time = 0;
int gathers = 0;

char *
get_cell(char *_grid, int row, int col)
{
	return &(_grid[n*row + col]);
}

/* Use only with small numbers of n for readability */
void
DisplayGoL()
{
	int row, col;
	struct timeval t_gather1, t_gather2;

	printf("Gathering\n");
	gettimeofday(&t_gather1, NULL);
	MPI_Gatherv(get_cell(grid, 1, 0), nrows*n, MPI_CHARACTER, display_grid,
	    recv_counts, displs_counts, MPI_CHARACTER, 0, MPI_COMM_WORLD);
	gettimeofday(&t_gather2, NULL);
	gather_time += (t_gather2.tv_sec-t_gather1.tv_sec)*1000 +
	    ((double) t_gather2.tv_usec-t_gather1.tv_usec)/1000;
	gathers++;
	printf("Done gathering\n");

	if (rank == 0) {
		for (row = 0; row < n; row++) {
			putchar('|');
			for (col = 0; col < n; col++) {
				printf("%c|",
				    *(get_cell(display_grid, row, col)));
			}
			putchar('\n');
		}
		printf("\n----------------------------------------------\n\n");
	}
}

void
set_pointers()
{
	/* first row in grid, actually controlled by previous proc */
	above_neighbors = get_cell(grid, 0, 0);
	/* last row in grid, actually controlled by next proc */
	below_neighbors = get_cell(grid, nrows + 1, 0);

	/* Top and bottom rows controlled by this chunk are 2nd from the top
	 * and bottom, respectively. */
	top_row = get_cell(grid, 1, 0);
	bottom_row = get_cell(grid, nrows, 0);
}

void
GenerateInitialGoL()
{
	int row, col, i, myseed = 0, seed = 0;
	MPI_Status status;


	/* Generate p random numbers as seeds for each process. */
	if (rank == 0) {
		/* Generate, send for each proc. */
		srand(time(NULL));

		/* Get proc 0's seed */
		myseed = rand() % BIG_PRIME;

		for (i = 1; i < p; i++) {
			seed = rand() % BIG_PRIME;
			/* Send seed to proc i */
			MPI_Send(&seed, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		}

		/* Re-seed with our randomly generated seed */
		srand(myseed);

	} else {
		/* Receive seeds from proc 0 */
		for (i = 1; i < p; i++) {
			if (rank == i) {
				/* Receive seed, seed rand */
				MPI_Recv(&myseed, 1, MPI_INT, 0, MPI_ANY_TAG,
				    MPI_COMM_WORLD, &status);

				srand(myseed);
			}
		}
	}

	/* Divide grid into chunks by row */
	nrows = n/p;
	/* Add room for first row above this chunk and first row below this
	 * chunk from neighboring procs */
	grid = (char *) malloc((nrows+2)*n);
	next_grid = (char *) malloc((nrows+2)*n);

	/* Populate grid, skipping neighbor cells since they are populated by
	 * other procs */
	for (row = 1; row <= nrows; row++) {
		for (col = 0; col < n; col++) {
			if (rand() % 2) {
				/* Odd, so cell is "dead" */
				*(get_cell(grid, row, col)) = ' ';
			}
			else {
				/* Cell is "alive" */
				*(get_cell(grid, row, col)) = '*';
			}
		}
	}
	set_pointers();

	/* figure out which proc has the rows "above" us and "below" us,
	 * remembering that the grid wraps. */
	above_rank = (rank + p - 1) % p;
	below_rank = (rank + 1) % p;
	printf("Above rank: %d, below rank: %d\n", above_rank, below_rank);

	display_grid = (char *) malloc(n*n);
	recv_counts = (int *) malloc(sizeof(int)*p);
	displs_counts = (int *) malloc(sizeof(int)*p);
	for (i = 0; i < p; i++) {
		recv_counts[i] = nrows*n;
		displs_counts[i] = i*nrows*n;
	}
}

void
get_above_neighbors()
{
	MPI_Status status;
	struct timeval t_recv1, t_recv2;

	gettimeofday(&t_recv1, NULL);
	MPI_Recv(above_neighbors, n, MPI_CHARACTER, above_rank, MPI_ANY_TAG,
	    MPI_COMM_WORLD, &status);
	gettimeofday(&t_recv2, NULL);
	recv_time += (t_recv2.tv_sec-t_recv1.tv_sec)*1000 +
	    ((double) t_recv2.tv_usec-t_recv1.tv_usec)/1000;
	recvs++;
}

void
get_below_neighbors()
{
	MPI_Status status;
	struct timeval t_recv1, t_recv2;

	gettimeofday(&t_recv1, NULL);
	MPI_Recv(below_neighbors, n, MPI_CHARACTER, below_rank, MPI_ANY_TAG,
	    MPI_COMM_WORLD, &status);
	gettimeofday(&t_recv2, NULL);
	recv_time += (t_recv2.tv_sec-t_recv1.tv_sec)*1000 +
	    ((double) t_recv2.tv_usec-t_recv1.tv_usec)/1000;
	recvs++;
}

void
send_top_row()
{
	struct timeval t_send1, t_send2;

	printf("Sending from %d to %d\n", rank, above_rank);
	gettimeofday(&t_send1, NULL);
	MPI_Send(top_row, n, MPI_CHARACTER, above_rank, 0, MPI_COMM_WORLD);
	gettimeofday(&t_send2, NULL);
	send_time += (t_send2.tv_sec-t_send1.tv_sec)*1000 +
	    ((double) t_send2.tv_usec-t_send1.tv_usec)/1000;
	sends++;
}

void
send_bottom_row()
{
	struct timeval t_send1, t_send2;

	printf("Sending from %d to %d\n", rank, below_rank);
	gettimeofday(&t_send1, NULL);
	MPI_Send(bottom_row, n, MPI_CHARACTER, below_rank, 0, MPI_COMM_WORLD);
	gettimeofday(&t_send2, NULL);
	send_time += (t_send2.tv_sec-t_send1.tv_sec)*1000 +
	    ((double) t_send2.tv_usec-t_send1.tv_usec)/1000;
	sends++;
}

char
DetermineState(int row, int col)
{
	int alive_neighbors = 0, left, right, above, below, alive;
	char *cell;
	/* Don't need to worry above wrapping above and below, since we have
	 * buffer rows on the top and bottom from neighboring procs. */
	above = row - 1;
	below = row + 1;
	left = (col + n - 1) % n;
	right = (col + 1) % n;

	/* Start at NW neighbor, go clockwise */
	if (*(get_cell(grid, above, left)) == '*')
		alive_neighbors++;
	if (*(get_cell(grid, above, col)) == '*')
		alive_neighbors++;
	if (*(get_cell(grid, above, right)) == '*')
		alive_neighbors++;
	if (*(get_cell(grid, row, right)) == '*')
		alive_neighbors++;
	if (*(get_cell(grid, below, right)) == '*')
		alive_neighbors++;
	if (*(get_cell(grid, below, col)) == '*')
		alive_neighbors++;
	if (*(get_cell(grid, below, left)) == '*')
		alive_neighbors++;
	if (*(get_cell(grid, row, left)) == '*')
		alive_neighbors++;

	cell = get_cell(grid, row, col);
	alive = (*cell == '*') ? 1 : 0;

	if (alive) {
		switch (alive_neighbors) {
		case 0:
		case 1:
			/* Cell dies from loneliness :( */
			return ' ';
		case 2:
		case 3:
			/* Cell lives another day */
			return '*';
		default:
			/* At least 4 alive neighbors, cell dies from
			 * overpopulation */
			return ' ';
		}
	}
	else {
		if (alive_neighbors == 3)
			/* Cell comes alive! */
			return '*';
	}

	/* Cell remains dead */
	return ' ';
}

void
Simulate()
{
	/* TODO: swap new and old grids! */
	int i, row, col;
	char *temp = NULL;
	struct timeval t_barrier1, t_barrier2;
	struct timeval t_display1, t_display2;

	for (i = 0; i < generations; i++) {
		/* Send current generation top and bottom rows so other procs
		 * can calculate next generation. */
		printf("Sending top and bottom rows");
		send_top_row();
		send_bottom_row();
		printf("Done sending top and bottom rows");
		/* Receive current generation above and below neighbors so we
		 * can calculate the next generation */
		printf("Getting top and bottom rows");
		get_above_neighbors();
		get_below_neighbors();
		printf("Done getting top and bottom rows");

		/* Calculate next generation */
		for (row = 1; row <= nrows; row++) {
			for (col = 0; col < n; col++) {
				/* calculate */
				*(get_cell(next_grid, row, col)) =
				    DetermineState(row, col);
			}
		}
		/* Swap grids for new generation */
		temp = grid;
		grid = next_grid;
		next_grid = temp;
		/* Reset pointers for top/bottom rows, etc since we are
		 * pointing to other grid now */
		set_pointers();

		gettimeofday(&t_barrier1, NULL);
		/* Keep procs on the same generation as each other */
		printf("Barrier time");
		MPI_Barrier(MPI_COMM_WORLD);
		printf("End barrier time");
		gettimeofday(&t_barrier2, NULL);
		barrier_time += (t_barrier2.tv_sec-t_barrier1.tv_sec)*1000 +
		    ((double) t_barrier2.tv_usec-t_barrier1.tv_usec)/1000;
		barriers++;

		if (i % 50 == 0) {
			gettimeofday(&t_display1, NULL);
			DisplayGoL();
			gettimeofday(&t_display2, NULL);
			display_time += (t_display2.tv_sec-t_display1.tv_sec)*1000 +
			    ((double) t_display2.tv_usec-t_display1.tv_usec)/1000;
			displays++;
		}
	}
}
