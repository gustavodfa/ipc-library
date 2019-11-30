#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h>
#include "../src/msgq.c"
int main() 
{ 
    int topicId;
    printf("What topic id you want to write for : "); 
    scanf("%d", &topicId);

    int data;
    printf("Write Data : "); 
    scanf("%d", &data);
    pubsub_init();
    int result = pubsub_publish(topicId, data);
    printf("Data written in memory: %d\n", result);
  
    return 0; 
} 
