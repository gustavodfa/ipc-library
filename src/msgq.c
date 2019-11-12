/*
 * Message queueing library for inter-process comunication on Linux systems.
 *
 * @author Gustavo Alves
 *
 */

#include "msgq.h"

key_t generateKey(int topicId) {
    key_t key;
    if ((key = ftok(SHR_MEM_FILE, topicId)) == -1) {
        perror("ftok error");
        exit(1);
    }
    return key;
}

// Creates the shrmem file to allow shared memory.
int pubsub_init(void) {
  FILE *fp;
  fp = fopen(SHR_MEM_FILE,"w");
  fclose(fp);
  return 0;
}

// 
int pubsub_create_topic(int topic_id) {
  // TODO: Function needs to be implemented
  return -1;
}

int pubsub_join(int topic_id) {
  // TODO: Function needs to be implemented
  return -1;
}

int pubsub_subscribe(int topic_id) {
  // TODO: Function needs to be implemented
  return -1;
}

int pubsub_cancel(int topic_id) {
  // TODO: Function needs to be implemented
  return -1;
}

int pubsub_publish(int topic_id, int msg) {
    key_t key = generateKey(topic_id); // Get key for the topic.
    int shmid = shmget(key,1024,0666|IPC_CREAT); // Get shmid
    int *memory = (int*) shmat(shmid,(void*)0,0); // Attach to memory
    memory = &msg;
    shmdt(memory);  // Detach from memory
    return *memory;
}

int pubsub_read(int topic_id) {
    key_t key = generateKey(topic_id); // Get key for the topic.
    int shmid = shmget(key,1024,0666|IPC_CREAT); // Get shmid
    int *memory = (int*) shmat(shmid,(void*)0,0); // Attach to memory
    int value = *memory;
    shmdt(memory);  // Detach from memory
    return value;
}


