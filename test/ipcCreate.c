#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h> 
#include "../src/msgq.c"
int main() 
{ 
    int topicId, bufSize;
    printf("What topic id you want to create: "); 
    scanf("%d", &topicId);
    printf("What buf size you want to have: ");
    scanf("%d", &bufSize);
    pubsub_init();
    int status = pubsub_create_topic(topicId, bufSize);
    if (status == -1) {
        printf("Couldn't initiate topic. :(\n");
        return 1;
    }
    printf("Topic %d was successfully initiated.\n", topicId);
    return 0;
} 