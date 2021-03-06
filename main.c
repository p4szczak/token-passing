#include <mpi.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#define MSG_TOKEN 100
#define MSG_ACK 101
#define HEARTBEAT 1
#define PROBABILITY_OF_LOSS 30

/*
 * -1 -> no token
 *  0 -> white token
 *  1 -> black token
 *  2 -> ack token
 */
int token = -1;
int shouldSend = 0;
int expectedToken = 0;

struct mpiData {
    int rank;
    int size;
};

pthread_mutex_t tokenMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shouldSendMutex = PTHREAD_MUTEX_INITIALIZER;


void *receive(void *input){
	time_t t;
	
	int size = ((struct mpiData*)input)->size;
	int rank = ((struct mpiData*)input)->rank;
	srand((unsigned) time(&t) + rank);
	int tempToken;
	MPI_Status status;
	int ack = rank + 2;
	int predecessor = (rank + (size - 1)) % size;

	while(1){
		// printf("|%d| I am waiting for a token\n", rank);
		MPI_Recv(&tempToken, 1, MPI_INT, predecessor, MPI_ANY_TAG, MPI_COMM_WORLD, &status );

		if((rand() % 100) < PROBABILITY_OF_LOSS){
			// printf("|%d| loss |%d| GOD DAMMIT!\tGOD DAMMIT!\tGOD DAMMIT!\n", rank, tempToken);
			continue;
		}
		// printf("|%d| Received token -> %d\n", rank, tempToken);

		// If received ack was sent by us (ack value equal to rank + 2, +2 is an encoding matter)
		if(tempToken == rank + 2){
			continue;
		}
		//if i am still sending tokens that means I have not receive acknowledge of recently send token
		if(shouldSend && (tempToken >= 2 || tempToken == expectedToken)){
			pthread_mutex_lock(&shouldSendMutex);
			shouldSend = 0;
			// printf("|%d| Received %d, stop sending\n", rank, tempToken);
			pthread_mutex_unlock(&shouldSendMutex);
			MPI_Send(&tempToken, 1, MPI_INT, (rank + 1) % size, MSG_ACK, MPI_COMM_WORLD);
		}
		//condition tempToken == ack is obligatory because node should not send token away, just acknowledges
		else if(!shouldSend && tempToken >= 2){
			MPI_Send(&tempToken, 1, MPI_INT, (rank + 1) % size, MSG_ACK, MPI_COMM_WORLD);
		}

		// printf("!!!!!!!!!!!!!!!!!!!!!|%d| Received |%d| expecting |%d|\n", rank, tempToken, expectedToken);
		if(tempToken == expectedToken){
			pthread_mutex_lock(&tokenMutex);
			token = tempToken;
			pthread_mutex_unlock(&tokenMutex);
			MPI_Send(&ack, 1, MPI_INT, (rank + 1) % size, MSG_ACK, MPI_COMM_WORLD);
		}
	}
}

void send(int token, int rank,int size){
	int sentToken = token;

	pthread_mutex_lock(&shouldSendMutex);
	shouldSend = 1;
	pthread_mutex_unlock(&shouldSendMutex);
	if(rank == 0){
		sentToken = (sentToken + 1) % 2;
	}
	while(shouldSend){
		MPI_Send(&sentToken, 1, MPI_INT, (rank + 1) % size, MSG_TOKEN, MPI_COMM_WORLD);
		sleep(HEARTBEAT);
		// printf("|%d| I am RETRANSMITTING a token %d\n", rank, sentToken);
	}
}

int main(int argc, char **argv){

	struct mpiData *mpiData = (struct mpiData*) malloc(sizeof(struct mpiData));
	
	// MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &(mpiData->rank));
	MPI_Comm_size(MPI_COMM_WORLD, &(mpiData->size));

	pthread_t receiveThread;
	pthread_create(&receiveThread, NULL, receive, (void *) mpiData);

	if(mpiData->rank == 0){
		token = 1;
		expectedToken = 1;
	}

	while(1){
		while(token == -1);
		expectedToken = (expectedToken + 1) % 2;
		printf("|%d| I am in critical section\n", mpiData->rank);
		sleep(3);
		
		send(token, mpiData->rank, mpiData->size);
		pthread_mutex_lock(&tokenMutex);
		token = -1;
		pthread_mutex_unlock(&tokenMutex);
	}
	free(mpiData);
	return 0;
}
