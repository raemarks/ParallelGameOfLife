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

char *
get_cell(char *_grid, int row, int col)
{
	return &(_grid[n*row + col]);
}

/* Use only with small numbers of n for readability */
void
DisplayGoL()
{
	int i, row, col;
	char *remote_grid = (char *) malloc(nrows*n);
	struct timeval t_send1, t_send2;
	struct timeval t_recv1, t_recv2;
	MPI_Status status;

	for(i = 0; i < p; i++) {
		if (rank == 0 && i == 0) {
			/* Print proc 0's grid, skip 1st and last rows since
			 * they don't belong to this proc */
			for (row = 1; row <= nrows; row++) {
				putchar('|');
				for (col = 0; col < n; col++) {
					printf("%c|", *(get_cell(grid, row, col)));
				}
				putchar('\n');
			}
		}
		else if (rank == 0) {
			/* Receive from proc i, print */
			gettimeofday(&t_recv1, NULL);
			MPI_Recv(remote_grid, nrows*n, MPI_CHARACTER, i,
			    MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			gettimeofday(&t_recv2, NULL);
			recv_time += (t_recv2.tv_sec-t_recv1.tv_sec)*1000 +
			    ((double) t_recv2.tv_usec-t_recv1.tv_usec)/1000;
			recvs++;

			for (row = 0; row < nrows; row++) {
				putchar('|');
				for (col = 0; col < n; col++) {
					printf("%c|",
					    *(get_cell(remote_grid, row, col)));
				}
				putchar('\n');
			}
		}
		else if (rank == i) {
			/* Rank == i != 0 */
			/* Send from proc i to proc 0, skip 1st and last row
			 * since it doesn't belong to this proc */
			gettimeofday(&t_send1, NULL);
			MPI_Send(top_row, nrows*n, MPI_CHARACTER, 0, 0,
			    MPI_COMM_WORLD);
			gettimeofday(&t_send2, NULL);
			send_time += (t_send2.tv_sec-t_send1.tv_sec)*1000 +
			    ((double) t_send2.tv_usec-t_send1.tv_usec)/1000;
			sends++;
		}
		/* Else don't do anything */
	}
	if (rank == 0)
		printf("\n----------------------------------------------\n\n");

	free(remote_grid);
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
	struct timeval t_recv1, t_recv2;
	struct timeval t_send1, t_send2;
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
	//		gettimeofday(&t_send1, NULL);
			MPI_Send(&seed, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
	/*		gettimeofday(&t_send2, NULL);
			send_time += (t_send2.tv_sec-t_send1.tv_sec)*1000 +
			    ((double) t_send2.tv_usec-t_send1.tv_usec)/1000;
			sends++;
			*/
		}

		/* Re-seed with our randomly generated seed */
		srand(myseed);

	} else {
		/* Receive seeds from proc 0 */
		for (i = 1; i < p; i++) {
			if (rank == i) {
				/* Receive seed, seed rand */
				//gettimeofday(&t_recv1, NULL);
				MPI_Recv(&myseed, 1, MPI_INT, 0, MPI_ANY_TAG,
				    MPI_COMM_WORLD, &status);
				/*
				gettimeofday(&t_recv2, NULL);
				recv_time += (t_recv2.tv_sec-t_recv1.tv_sec)*1000 +
				    ((double) t_recv2.tv_usec-t_recv1.tv_usec)/1000;
				recvs++;
				*/

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
		if (rank == 0)
			printf("Gen %d\n", i);

		/* Send current generation top and bottom rows so other procs
		 * can calculate next generation. */
		send_top_row();
		send_bottom_row();
		/* Receive current generation above and below neighbors so we
		 * can calculate the next generation */
		get_above_neighbors();
		get_below_neighbors();

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
		MPI_Barrier(MPI_COMM_WORLD);
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
