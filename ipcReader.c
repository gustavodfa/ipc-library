#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h> 
#include "./ipcLib.c"
int main() 
{ 
    int topicId;
    printf("What topic id you want to read from : "); 
    scanf("%d", &topicId);
    int value = read(topicId);
    printf("Data read from memory: %d\n", value);
    return 0; 
} 