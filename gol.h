#ifndef _GOL_H_
#define _GOL_H_

extern char *grid;
extern char *bottom_neighbors;
extern char *top_neighbors;
extern int rank;
extern int p;
extern int n;
extern int generations;

extern double total_runtime;
extern double display_time;
extern int displays;
extern double send_time;
extern int sends;
extern double recv_time;
extern int recvs;
extern double barrier_time;
extern int barriers;
extern double bcast_time;
extern int bcasts;
extern double gather_time;

void GenerateInitialGoL();
void Simulate();

#endif
