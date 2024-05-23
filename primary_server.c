#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <semaphore.h>
 

#define MSGKEY 1000
#define TERMINATION_MESSAGE 199
#define SHMKEY 1234

typedef struct {
    long mtype;
    int sequenceNumber;
    int operationNumber;
    char graphFile[100]; 
} request; 

typedef struct {
    int nodes; 
    int adjMatrix[100][100]; 
} SharedMemory;

typedef struct {
    long mtype;
    char responseText[256];
} response;

void * handleRequest(void *arg);

sem_t *mutex; 

int main(){
    int loop = 1;
    
    int msgid = msgget((key_t)MSGKEY, 0); 

    sem_unlink("/server_mutex");    
    mutex = sem_open("/server_mutex", O_CREAT | O_EXCL, 0666, 1); 
    
    if(mutex == SEM_FAILED){
        perror("Mutex Error");
        exit(1);
    }

    while(loop){
        request clientmsg;
        if(msgrcv(msgid, &clientmsg, sizeof(clientmsg), 1, 0) < 0){
            perror("msgrcv error");
            exit(1);
        }

        if(clientmsg.sequenceNumber == -1){
            printf("Termination Message Received, exiting server.\n");
            exit(0); 
        }

        printf("Request %ld received %s.\n", clientmsg.mtype, clientmsg.graphFile);

        pthread_t thread;
        if(pthread_create(&thread, NULL, handleRequest, (void*)&clientmsg) != 0){
            perror("error creating thread");
            exit(1);
        }

        pthread_join(thread, NULL);
    }

    sem_close(mutex);
    sem_unlink("/server_mutex");


    return 0; 
}

void * handleRequest(void *arg){
    int msgid = msgget((key_t)MSGKEY, 0); 

    request* clientmsg = (request *)arg; 

    int shm_id = shmget(SHMKEY, sizeof(SharedMemory), 0);
    SharedMemory *shm = (SharedMemory *) shmat(shm_id, NULL, 0);

    int nodes = shm->nodes; 
    int adjMatrix[nodes][nodes];
    for(int i = 0; i < nodes; i++){
        for(int j = 0 ; j < nodes ; j++){
            adjMatrix[i][j] = shm->adjMatrix[i][j];
        }
    }

    shmdt(shm);
    // shmctl(shm_id, IPC_RMID, NULL);
    printf("entereeed here.\n");
    sem_wait(mutex);
    printf("entereeed here inside.\n");

    FILE* graphFile;  

    response responseMessage;  
    
    if(clientmsg->operationNumber == 1){
        graphFile = fopen(clientmsg->graphFile, "w");
        strcpy(responseMessage.responseText,"File Successfully Added");
    } else {
        graphFile = fopen(clientmsg->graphFile, "a");
        strcpy(responseMessage.responseText,"File Successfully Modified");
    }

    for(int i = 0; i < nodes; i++){
        for(int j = 0; j < nodes ; j++){
            fprintf(graphFile, "%d ", adjMatrix[i][j]);
        }
        fprintf(graphFile, "\n"); 
    }

    fclose(graphFile);

    sem_post(mutex); 

    // responseMessage.mtype = clientmsg->sequenceNumber;
    responseMessage.mtype = (long)clientmsg->sequenceNumber;

    msgsnd(msgid, &responseMessage, sizeof(response), 0);
    printf("Response sent to %ld,  %s\n.", responseMessage.mtype, responseMessage.responseText); 

    pthread_exit(NULL); 
}