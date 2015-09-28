#ifndef _GOL_H_
#define _GOL_H_

extern char *grid;
extern char *bottom_neighbors;
extern char *top_neighbors;
extern int rank;
extern int p;
extern int n;
extern int generations;

void GenerateInitialGoL();
void Simulate();

#endif
