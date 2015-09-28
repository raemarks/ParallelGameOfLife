all:
	mpicc -Wall -lm -o gol main.c gol.c

run:
	mpiexec -machinefile ./machinefile.txt -n 5 ./gol

send:
	rsync machinefile.txt main.c gol.c gol.h Makefile rmarks@ssh1.eecs.wsu.edu:/net/u/rmarks/pvt/
