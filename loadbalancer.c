#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h> 

#define MSGKEY 1000
#define TERMINATION_MESSAGE 999


typedef struct {
    long mtype;
    int sequenceNumber;
    int operationNumber;
    char graphFile[100]; 
} request; 

typedef struct {
    long mtype;
    char responseText[256];
} response;


int main(){
    
    int msgid = msgget((key_t)MSGKEY, IPC_CREAT | 0666);
    printf("Queue created by Load Balancer. \n");
    int loop = 1; 

    while(loop) {
        request client_request; 
        msgrcv(msgid, &client_request, sizeof(client_request), 0, 0);

        if(client_request.mtype == TERMINATION_MESSAGE){
            for(long i = 1 ; i <= 3; i++ ){
                client_request.mtype = i; 
                client_request.sequenceNumber = -1; 
                msgsnd(msgid, &client_request, sizeof(client_request), 0);
            }
            sleep(5); 
            printf("Termination Message Received, exiting.\n");
            msgctl(msgid, IPC_RMID, NULL);
            exit(0);
        }

        long server; 

        if(client_request.operationNumber == 1 || client_request.operationNumber == 2) {
            server = 1; 
        } else {
            server = (client_request.sequenceNumber % 2 == 0) ? 3 : 2;
        }


        client_request.mtype = server; 
        printf("Message Received %d. Forwarding to Server: %ld \n", client_request.sequenceNumber, server);
         
        if(msgsnd(msgid, &client_request, sizeof(client_request), 0) < 0){
            perror("msgsnd error");
            exit(1); 
        }

    }
    
    msgctl(msgid, IPC_RMID, NULL);
    return 0; 
}   