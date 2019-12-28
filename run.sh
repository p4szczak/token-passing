mpicc -pthread -Wall -o main main.c
mpirun -np $1 ./main
