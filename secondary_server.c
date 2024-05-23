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
#define SHMKEY 1234
#define NODES 30
#define TERMINATION_MESSAGE 199
#define MAX_THREADS 60

typedef struct {
    long mtype;
    int sequenceNumber;
    int operationNumber;
    char graphFile[100]; 
} request; 

typedef struct {
    int vertices[NODES];
    int length;
} Path; 

typedef struct {
    int adjMatrix[NODES][NODES]; 
    int nodes; 
    int currentVertex;
    int visitedNodes[NODES];
    Path *path; 
} ThreadArgs; 

typedef struct {
    int vertex;
    int level; 
} bfsNode; 

typedef struct {
    int nodes; 
    int adjMatrix[100][100]; 
} SharedMemory;

typedef struct {
    long mtype;
    char responseText[256];
} response;

int bfsVisitedNodes[NODES] = {0};
int dfsVisitedNodes[NODES] = {0};

sem_t *mutex;

bfsNode * queue;
int front, rear; 

int threadBFS = 0;

void printPath(Path *path){
    for(int i = 0 ; i < path->length; i++){
        printf("%d ", path->vertices[i]);
    }
    printf("\n");
}


void * dfstraversal(void *args){
    ThreadArgs *threadArgs = (ThreadArgs *)args; 


    Path *vertexPath = threadArgs->path;

    int numberofNodes = threadArgs->nodes; 
    int vertex = threadArgs->currentVertex; 
    int threadDFS = 0; 
    // printf("New node: %d\n", vertex); 
    pthread_t dfsTraverse[numberofNodes];

    vertexPath->vertices[vertexPath->length++] = vertex; 
    // threadArgs->visitedNodes[vertex] = 1; 
    dfsVisitedNodes[vertex] = 1;

    printPath(vertexPath);
    
    for(int i = 0; i < numberofNodes; i++){
        if(threadArgs->adjMatrix[vertex][i] == 1 && dfsVisitedNodes[i] == 0){
ThreadArgs recurseArgs; 

            for(int i = 0 ; i < numberofNodes; i++){
                for(int j = 0; j < numberofNodes; j++){ 
                    recurseArgs.adjMatrix[i][j] = threadArgs->adjMatrix[i][j];

                }
            }


            recurseArgs.nodes = numberofNodes;
            recurseArgs.currentVertex = i;
            recurseArgs.path = vertexPath; 

            if(pthread_create(&dfsTraverse[threadDFS++], NULL, dfstraversal, (void *)&recurseArgs) != 0){
                perror("dfs thread error");
                exit(1); 
            }
        }

        for(int i = 0 ; i < threadDFS; i++){
            pthread_join(dfsTraverse[i], NULL);
        }
    }
    
    for(int i = 0 ; i < threadDFS; i++){
        pthread_join(dfsTraverse[i], NULL);
    }

    pthread_exit(NULL); 
}

void enqueue(bfsNode * queue, int *rear, bfsNode node){
    // printf("Entered Enqueue and Node: %d\n", node.vertex);
    (*rear)++;
    queue[*rear] = node; 
}

bfsNode dequeue(bfsNode * queue, int *front){
    
    if(*front < 0){
        bfsNode nullNode = {-1, -1};
        return nullNode; 
    }

    bfsNode node = queue[*front];
    (*front)++;
    return node; 
}


void printQueue(bfsNode queue[], int front, int rear) {
    if (front == -1 || front > rear) {
        printf("Queue is empty\n");
        return;
    }

    printf("Queue: ");
    for (int i = front; i <= rear; i++) {
        printf("(%d, %d) ", queue[i].vertex, queue[i].level);
    }
    printf("\n");
}



void *bfstraversal(void *args){

    ThreadArgs* threadArgs = (ThreadArgs*)args;
    int numberofNodes = threadArgs->nodes; 

    Path *vertexPath = threadArgs->path; 

    int vertex = threadArgs->currentVertex;
    // printf("Current Vertex: %d\n", vertex);
    int currentLevel; 
    pthread_t bfsTraverse[numberofNodes];
    

    if(vertexPath->length == 0){  
        bfsNode firstNode = {vertex, 0}; 
        currentLevel = 0; 
        enqueue(queue, &rear, firstNode); 
        // printQueue(queue, front, rear);
        vertexPath->vertices[vertexPath->length++] = vertex;

    }


    bfsVisitedNodes[vertex] = 1;

    printf("Current Node: %d\n", vertex );


    while(queue[front].level == currentLevel || front < rear){
        bfsNode currentNode = dequeue(queue, &front);
        // printf("Entered while loop. %d node: %d\n", currentLevel, currentNode.vertex);   
        printf("N Node %d enqueued.Level %d and threadCount: %d\n", currentNode.vertex, currentNode.level, threadBFS);
        ThreadArgs recurseArgs; 


        for(int i = 0 ; i < numberofNodes; i++){
            for(int j = 0; j < numberofNodes; j++){ 
                recurseArgs.adjMatrix[i][j] = threadArgs->adjMatrix[i][j];
            }
        }

        recurseArgs.nodes = numberofNodes;
        // for(int i =0 ; i < numberofNodes; i++){
        //     recurseArgs.visitedNodes[i] = threadArgs->visitedNodes[i]; 
        // }

        currentLevel++; 
        threadBFS++;  
        for(int i = 0; i < numberofNodes ; i++){
            if(bfsVisitedNodes[i] == 0 && threadArgs->adjMatrix[currentNode.vertex][i] == 1){

                bfsNode neighborNode = {i, currentNode.level + 1};
                // printf("\nNeigbor Found: %d of node %d.\n", neighborNode.vertex, currentNode.vertex);
                
                enqueue(queue, &rear, neighborNode);
                // printf("enqueued: %d\n", queue[front].vertex);
                vertexPath->vertices[vertexPath->length++] = neighborNode.vertex;
                // printPath(vertexPath); 

                bfsVisitedNodes[i] = 1;

                recurseArgs.currentVertex = i;

                recurseArgs.path = vertexPath; 

                // printQueue(queue, front, rear);

                if(pthread_create(&bfsTraverse[threadBFS], NULL, bfstraversal, (void *)&recurseArgs) != 0){
                    perror("dfs thread error");
                    exit(1); 
                }
            }
        }

        for(int i = 0 ; i < threadBFS; i++){
            pthread_join(bfsTraverse[i], NULL);

        }
    }

    pthread_exit(NULL);
}

void *handleRequest(void *args){
    request* client_request = (request *)args; 
    int msgid = msgget((key_t)MSGKEY, 0); 

    int shm_id = shmget(SHMKEY, sizeof(SharedMemory), 0);
    SharedMemory *shm = (SharedMemory *) shmat(shm_id, NULL, 0);

    int vertex = shm->nodes; 
    printf("\n\nSelected Vertex Node: %d\n", vertex); 

    threadBFS = 0;

    shmdt(shm);

    sem_wait(mutex); 
    printf("Mutex locked./n");

    FILE* file = fopen(client_request->graphFile, "r"); 

    if(file == NULL){
        perror("Error Opening File.");

        response errorMsg; 
        errorMsg.mtype = client_request->sequenceNumber;

        strcpy(errorMsg.responseText, "No file Found");
        msgsnd(msgid, &errorMsg, sizeof(errorMsg), 0);

        pthread_exit(NULL);
    }

    int numberofNodes = 0; 
    while(!feof(file)){
        if(fgetc(file) == '\n'){
            numberofNodes++; 
        }
    }

    fseek(file, 0, SEEK_SET);

    sem_post(mutex);

    printf("Number of Nodes found: %d\n", numberofNodes);

    threadBFS = 0;

    ThreadArgs threadArgs; 
    threadArgs.nodes = numberofNodes;
    threadArgs.currentVertex = vertex;
    memset(threadArgs.visitedNodes, 0, sizeof(threadArgs.visitedNodes)); 
    memset(bfsVisitedNodes, 0, sizeof(bfsVisitedNodes));
    memset(dfsVisitedNodes, 0, sizeof(dfsVisitedNodes));
    // int matrix[numberofNodes][numberofNodes];

    for(int i = 0 ; i < numberofNodes; i++){
        for(int j = 0; j < numberofNodes; j++){
            fscanf(file, "%d", &threadArgs.adjMatrix[i][j]); 
            // printf("%d ", threadArgs.adjMatrix[i][j]);
        }
        // printf("\n");
    }

    pthread_t dfsThread, bfsThread;

    Path vertexPath;

    vertexPath.length = 0;
    memset(vertexPath.vertices, 0, sizeof(vertexPath.vertices));
    if(client_request->operationNumber == 3){ //dfs

        threadArgs.path = &vertexPath; 


        if(pthread_create(&dfsThread, NULL, dfstraversal, (void *)&threadArgs) != 0){
            perror("thread error");
            exit(1);
        }

        pthread_join(dfsThread, NULL);

        response dfsResponse; 

        dfsResponse.mtype = client_request->sequenceNumber; 

        for(int i =0 ; i < numberofNodes; i++){
            dfsResponse.responseText[i * 2] = (vertexPath.vertices[i] + '0');
            dfsResponse.responseText[i * 2 + 1] = ' ';
        } 

        printf("DFS Traversal: %s\n", dfsResponse.responseText);

    
        if(msgsnd(msgid, &dfsResponse, sizeof(response), 0) > 0){
            perror("msgsnd");
            exit(1);
        }

    } else if(client_request->operationNumber == 4){

        threadArgs.path = &vertexPath; 
        front = 0;
        rear = -1; 
        queue = (bfsNode*)malloc(numberofNodes * sizeof(bfsNode));

        if(pthread_create(&bfsThread, NULL, bfstraversal, (void *)&threadArgs) != 0){
            perror("thread error");
            exit(1);
        }

        pthread_join(bfsThread, NULL);

        response bfsResponse; 
        bfsResponse.mtype = client_request->sequenceNumber; 

        for(int i =0 ; i < numberofNodes; i++){
            bfsResponse.responseText[i * 2] = (vertexPath.vertices[i] + '0');
            bfsResponse.responseText[i * 2 + 1] = ' ';
        } 

        printf("BFS Traversal: %s\n", bfsResponse.responseText);
        
        if(msgsnd(msgid, &bfsResponse, sizeof(response), 0) > 0){
            perror("msgsnd");
            exit(1);
        }
    }

}

int main(int argc, char *argv[]){

    if(argc != 2){
        fprintf(stderr, "Incorrect number of args.\n");
        exit(1);    
    }

    int serverSelect = atoi(argv[1]);
    printf("Secondary Server %d started.\n", serverSelect ); 

    int loop = 1;
    int msgid = msgget((key_t)MSGKEY, IPC_CREAT | 0666);
    
    sem_unlink("/server_mutex");

    mutex = sem_open("/server_mutex", O_CREAT | O_EXCL, 0666, 1); 

    if(mutex == SEM_FAILED){
        perror("Mutex Error");
        exit(1);
    }

    while(loop){       
        request client_request;

        if((msgrcv(msgid, &client_request, sizeof(client_request), serverSelect + 1, IPC_NOWAIT) >= 0) ){
                
            sem_unlink("/server_mutex");
            
            if(client_request.sequenceNumber == -1){
                printf("Termination Message Received, exiting server.\n");
                sem_close(mutex);
                sem_unlink("/server_mutex");
                pthread_exit(NULL);
                exit(0);
            }

            printf("Message Received.\n"); 

            pthread_t thread;  
            if(pthread_create(&thread, NULL, handleRequest, (void*)&client_request) != 0){
                perror("error creating thread");
                exit(1);
            }
            pthread_join(thread, NULL);
        }
    }

    sem_close(mutex);
    sem_unlink("/server_mutex");

    return 0; 
}
