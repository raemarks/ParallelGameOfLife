all:
	mpicc -Wall -lm -o gol main.c gol.c

run:
	mpiexec -machinefile ./machinefile.txt -n 8 ./gol 32 100

test:
	mpiexec -machinefile ./machinefile.txt -n 1 ./gol 4 100
	mpiexec -machinefile ./machinefile.txt -n 1 ./gol 8 100
	mpiexec -machinefile ./machinefile.txt -n 1 ./gol 16 100
	mpiexec -machinefile ./machinefile.txt -n 1 ./gol 32 100
	mpiexec -machinefile ./machinefile.txt -n 1 ./gol 64 100
	mpiexec -machinefile ./machinefile.txt -n 1 ./gol 128 100
	mpiexec -machinefile ./machinefile.txt -n 1 ./gol 256 100
	mpiexec -machinefile ./machinefile.txt -n 1 ./gol 512 100
	mpiexec -machinefile ./machinefile.txt -n 1 ./gol 102 100
	mpiexec -machinefile ./machinefile.txt -n 1 ./gol 2048 100
	mpiexec -machinefile ./machinefile.txt -n 2 ./gol 4 100
	mpiexec -machinefile ./machinefile.txt -n 2 ./gol 8 100
	mpiexec -machinefile ./machinefile.txt -n 2 ./gol 16 100
	mpiexec -machinefile ./machinefile.txt -n 2 ./gol 32 100
	mpiexec -machinefile ./machinefile.txt -n 2 ./gol 64 100
	mpiexec -machinefile ./machinefile.txt -n 2 ./gol 128 100
	mpiexec -machinefile ./machinefile.txt -n 2 ./gol 256 100
	mpiexec -machinefile ./machinefile.txt -n 2 ./gol 512 100
	mpiexec -machinefile ./machinefile.txt -n 2 ./gol 1024 100
	mpiexec -machinefile ./machinefile.txt -n 2 ./gol 2048 100
	mpiexec -machinefile ./machinefile.txt -n 4 ./gol 4 100
	mpiexec -machinefile ./machinefile.txt -n 4 ./gol 8 100
	mpiexec -machinefile ./machinefile.txt -n 4 ./gol 16 100
	mpiexec -machinefile ./machinefile.txt -n 4 ./gol 32 100
	mpiexec -machinefile ./machinefile.txt -n 4 ./gol 64 100
	mpiexec -machinefile ./machinefile.txt -n 4 ./gol 128 10
	mpiexec -machinefile ./machinefile.txt -n 4 ./gol 256 10
	mpiexec -machinefile ./machinefile.txt -n 4 ./gol 512 10
	mpiexec -machinefile ./machinefile.txt -n 4 ./gol 1024 100
	mpiexec -machinefile ./machinefile.txt -n 4 ./gol 2048 100
	mpiexec -machinefile ./machinefile.txt -n 8 ./gol 8 100
	mpiexec -machinefile ./machinefile.txt -n 8 ./gol 16 100
	mpiexec -machinefile ./machinefile.txt -n 8 ./gol 32 100
	mpiexec -machinefile ./machinefile.txt -n 8 ./gol 64 100
	mpiexec -machinefile ./machinefile.txt -n 8 ./gol 128 100
	mpiexec -machinefile ./machinefile.txt -n 8 ./gol 256 100
	mpiexec -machinefile ./machinefile.txt -n 8 ./gol 512 100
	mpiexec -machinefile ./machinefile.txt -n 8 ./gol 1024 100
	mpiexec -machinefile ./machinefile.txt -n 8 ./gol 2048 100
	mpiexec -machinefile ./machinefile.txt -n 16 ./gol 16 100
	mpiexec -machinefile ./machinefile.txt -n 16 ./gol 32 100
	mpiexec -machinefile ./machinefile.txt -n 16 ./gol 64 100
	mpiexec -machinefile ./machinefile.txt -n 16 ./gol 128 100
	mpiexec -machinefile ./machinefile.txt -n 16 ./gol 256 100
	mpiexec -machinefile ./machinefile.txt -n 16 ./gol 512 100
	mpiexec -machinefile ./machinefile.txt -n 16 ./gol 1024 100
	mpiexec -machinefile ./machinefile.txt -n 16 ./gol 2048 100

send:
	rsync machinefile.txt main.c gol.c gol.h Makefile rmarks@ssh1.eecs.wsu.edu:/net/u/rmarks/pvt/
