#include <mpi.h>
#include <stdio.h>
#include <pthread.h>

/*
 * -1 -> brak tokenu
 *  0 -> bialy token
 *  1 -> czarny token
 *  2 -> potwierdzenie odebrania tokenu
 */
int token = -1;
int shouldSend = 0;
int expectedToken = 0;

typedef struct {
    int *rank;
    int *size;
} mpiData;

pthread_mutex_t tokenMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shouldSendMutex = PTHREAD_MUTEX_INITIALIZER;


void *receive(){
	int tempToken;

	MPI_Recv(&tempToken, 1, MPI_INT, (rank + (size - 1)) % size, MPI_ANY_TAG, MPI_COMM_WORLD, &status );	
	//kazda wiadomosc konczy wysylanie
	pthread_mutex_lock(&shouldSendMutex);
	shouldSend = 0;
	pthread_mutex_unlock(&shouldSendMutex);

	if(tempToken == expectedToken){
		pthread_mutex_lock(&tokenMutex);
		token = tempToken;
		pthread_mutex_unlock(&tokenMutex);
	}
}

void send(int token, int rank,int size){
	int sendedToken = token;
	pthread_mutex_lock(&shouldSendMutex);
	shouldSend = 1;
	pthread_mutex_unlock(&shouldSendMutex);
	if(rank == 0){
		sendedToken = (sendedToken + 1) % 2;
	}
	while(shouldSend){
		MPI_Send(sendedToken, 1, MPI_INT, (rank + 1) % size, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	}
}

int main(int argc, char **argv){
	pthread_t receiveThread;
	pthread_create(&receiveThread, NULL, receive, (void *) )
	int rank,size,sender;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(rank == 0){
		token = 0;
		expectedToken = 1;
	}

	while(1){
		while(token == -1);
		expectedToken = (expectedToken + 1) % 2 
		printf("|%d| I am in critical section");
		sleep(1);
		send(token, rank, size, status);
		pthread_mutex_lock(&tokenMutex)
		token = -1;
		pthread_mutex_unlock(&tokenMutex);
	}
}
