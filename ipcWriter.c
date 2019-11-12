#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h>
#include "./ipcLib.c"
int main() 
{ 
    int topicId;
    printf("What topic id you want to write for : "); 
    scanf("%d", &topicId);

    int data;
    printf("Write Data : "); 
    scanf("%d", &data);

    int written = write(topicId, data);
    printf("Data written in memory: %d\n", written);
  
    return 0; 
} 
