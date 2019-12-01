#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h> 
#include "../src/msgq.c"
int main() 
{ 
    printf("Reader: %d\n", getpid());
    int topicId;
    printf("What topic id you want to read from : "); 
    scanf("%d", &topicId);
    pubsub_init();
    int status = pubsub_subscribe(topicId);
    if (status == -1) {
        printf("didn't join\n");
        return -1;
    }
    while (1) {
        int option;
        printf("1 - Read. 2 - Quit.");
        scanf("%d", &option);
        if (option == 2) {
            return 0;
        }

        int value = pubsub_read(topicId);
        if (value == -1) {
            printf("no data to be read.\n");
        } else {
            printf("Data read from memory: %d\n", value);
        }
    }
    return 0;
} 