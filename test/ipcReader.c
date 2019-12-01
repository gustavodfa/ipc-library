#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h> 
#include "../src/msgq.c"
int main() 
{ 
    int topicId;
    printf("What topic id you want to read from : "); 
    scanf("%d", &topicId);
    pubsub_init();
    int status = pubsub_subscribe(topicId);
    if (status == -1) {
        printf("didn't join\n");
        return -1;
    }
    int value = pubsub_read(topicId);
    printf("Data read from memory: %d\n", value);
    return 0;
} 