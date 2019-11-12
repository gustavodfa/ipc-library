#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h>
#include <stdlib.h>

key_t generateKey(int topicId) {
    key_t key;
    if ((key = ftok("./shm", topicId)) == -1) {
        perror("ftok error");
        exit(1);
    }
    return key;
}

// Read from topic
int read(int topicId) {
    key_t key = generateKey(topicId); // Get key for the topic.
    int shmid = shmget(key,1024,0666|IPC_CREAT); // Get shmid
    int *memory = (int*) shmat(shmid,(void*)0,0); // Attach to memory
    int value = *memory;
    shmdt(memory);  // Detach from memory
    return value;
}

// Writes to topic
int write(int topicId, int value) {
    key_t key = generateKey(topicId); // Get key for the topic.
    int shmid = shmget(key,1024,0666|IPC_CREAT); // Get shmid
    int *memory = (int*) shmat(shmid,(void*)0,0); // Attach to memory
    memory[0] = &value;
    shmdt(memory);  // Detach from memory
    return *memory;
}