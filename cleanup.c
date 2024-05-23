#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <ctype.h> 
#include <signal.h>
#include <sys/wait.h>


#define TERMINATION_MESSAGE 999
#define MSGKEY 1000

typedef struct {
    long mtype;
    int sequenceNumber;
    int operationNumber;
    char graphFile[100]; 
} request; 

int main(){
    int msgid = msgget(MSGKEY, 0);
    if(msgid == -1){
        perror("msgget");
        exit(1);
    }

    while(1){
        printf("Do you want to terminate? Press Y for Yes and N for No: ");

        char input[10];
        fgets(input, sizeof(input), stdin);

        if(toupper(input[0]) == 'Y'){
            request termination;
            termination.mtype = TERMINATION_MESSAGE;
            termination.sequenceNumber = TERMINATION_MESSAGE;
            termination.operationNumber = -1; 

            strcpy(termination.graphFile, "T");

            if(msgsnd(msgid, &termination, sizeof(termination), 0) < 0){
                perror("msgsnd");
                exit(1);
            }
            // wait(NULL);
            break;
        } else if (toupper(input[0]) == 'N') {
            continue;
        } else {
            printf("Invalid input.\n");
            continue;
        }

    }
    return 0;
}o-l1