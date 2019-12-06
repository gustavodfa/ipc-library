#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h>
#include "../src/msgq.c"
int main() 
{ 
    printf("Writer: %d\n", getpid());
    int topicId;
    printf("What topic id you want to write for : "); 
    scanf("%d", &topicId);
    pubsub_init();
    int status = pubsub_join(topicId);
    if (status == -1) {
        printf("didn't join\n");
        return -1;
    }

    while (1) {
        int option;
        printf("1 - Write. 2 - Quit.");
        scanf("%d", &option);
        if (option == 2) {
            break;
        }
        
        int data;
        printf("Write Data : "); 
        scanf("%d", &data);
        int result = pubsub_publish(topicId, data);
        if (result == -1) {
            printf("error trying to write, try again.\n");
        } else {
            printf("Data written in memory: %d\n", result);
        }
    }
    int cancel = pubsub_cancel(topicId);
    if (cancel < 0) {
        printf("fail\n");
    }
    return 0; 
} 
