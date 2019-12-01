#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h>
#include "../src/msgq.c"
int main() 
{ 
    int topicId;
    printf("What topic id you want to write for : "); 
    scanf("%d", &topicId);
    pubsub_init();
    int status = pubsub_join(topicId);
    if (status == -1) {
        printf("didn't join\n");
        return -1;
    }
    int data;
    printf("Write Data : "); 
    scanf("%d", &data);
    int result = pubsub_publish(topicId, data);
    printf("Data written in memory: %d\n", result);
  
    return 0; 
} 
