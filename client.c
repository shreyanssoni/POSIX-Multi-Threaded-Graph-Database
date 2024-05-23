#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>

#define MSGKEY 1000
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

int main(){
    int msgid = msgget((key_t)MSGKEY, 0666 );
    int loop = 1; 

    while(loop){
        request client_request; 

        int shm_id = shmget(SHMKEY, sizeof(SharedMemory), 0666 | IPC_CREAT);
        SharedMemory *shm = (SharedMemory *) shmat(shm_id, NULL, 0);

        printf("Following are the operations:\n");
        printf("1. Add a new graph to the database\n");
        printf("2. Modify an existing graph of the database\n");
        printf("3. Perform DFS on an existing graph of the database\n");
        printf("4. Perform BFS on an existing graph of the database\n");

        printf("Enter Sequence Number: ");
        scanf("%d", &client_request.sequenceNumber);

        if(client_request.sequenceNumber > 100 || client_request.sequenceNumber <= 0){
            printf("Sequence Number can't be this value. re-enter:\n\n");
            continue; 
        }

        printf("Enter operation number: ");
        scanf("%d", &client_request.operationNumber);

        printf("Enter Graph File Name: ");
        scanf("%s", client_request.graphFile);

        client_request.mtype = (long)client_request.sequenceNumber; 
        // client_request.mtype = ; 

        if(client_request.operationNumber == 1 || client_request.operationNumber == 2){
            printf("Enter number of nodes of the graph: ");
            scanf("%d", &(shm->nodes));

            printf("Enter Adjancey Matrix, each row on a separate line and elements of a single row separated by whitespace characters: \n"); 
            for( int i =0 ; i < shm->nodes; i++){
                for(int j =0 ; j < shm->nodes; j++){
                    scanf("%d", &(shm->adjMatrix[i][j])); 
                }
            }
        } else if(client_request.operationNumber == 3 || client_request.operationNumber == 4) {
            printf("Enter starting Vertex for traversal: ");
            scanf("%d", &(shm->nodes));
        } else {
            printf("Incorrect Operation Number. Re-enter: \n\n");
            continue; 
        }

        msgsnd(msgid, &client_request, sizeof(request), 0);

        response serverResponse; 

        if(msgrcv(msgid, &serverResponse, sizeof(response), (long)client_request.sequenceNumber , 0) < 0 ){
            perror("msgrcv error");
            exit(1); 
        }
        
        printf("Server response: %s\n", serverResponse.responseText); 

        shmdt(shm);
        shmctl(shm_id, IPC_RMID, NULL);
    }   
    
    return 0; 

}