/*
 * Message queueing library for inter-process comunication on Linux systems.
 *
 * @author Gustavo Alves
 * @author Marcos Barros
 *
 */

#include "msgq.h"
#include "Shmem.c"
#include "string.h"

key_t generateKey(int topicId) {
    key_t key = ftok(SHR_MEM_FILE, topicId);
    if (key == -1) {
        perror("ftok error");
        exit(1);
    }
    return key;
}

int segmentId(key_t key) {
  int shmid = shmget(key, sizeof(Shmem), IPC_CREAT|0777);
  if (shmid == -1) {
    perror("shmget");
    exit(1);
  }
  return shmid;
}

void* attach(int shmid){
  void *shm = shmat(shmid, (void*)0, 0); // Attach to memory
  if (shm == (void *)(-1)) {
    perror("shmat");
    exit(1);
  }
  return shm;
}

// Creates the shrmem file to allow key generation.
int pubsub_init(void) {
  FILE *fp;
  fp = fopen(SHR_MEM_FILE,"w");
  fclose(fp);
  return 0;
}

// pubsub_create_topic initiates a topic with topic_id and bufSize.
int pubsub_create_topic(int topic_id, int bufSize) {
  Shmem init;
  init.lastMessage = -1;
  init.size = bufSize;
  memset(init.queue, 0, sizeof(init.queue));

  key_t key = generateKey(topic_id); // Get key for the topic.
  int shmid = segmentId(key);
  // Try to get a segment id, if already exists, delete it.
  Shmem *shm = attach(shmid); // Attach to memory
  *shm = init;
  shmdt(shm);
  return 1;
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

// pubsub_publish will write to a topic id and return -1 if action couldn't be performed.
int pubsub_publish(int topic_id, int msg) {
    key_t key = generateKey(topic_id); // Get key for the topic.
    int shmid = segmentId(key); // Get shmid
    Shmem *shm = attach(shmid);
    int status = add_message(shm, msg); //Write
    shmdt(shm);  // Detach from memory
    if (status == -1){
      return status;
    }
    return msg;
}

// pubsub_read will read last message from topic id and return -1 if no message is found.
int pubsub_read(int topic_id) {
    int msg;
    key_t key = generateKey(topic_id); // Get key for the topic.
    int shmid = segmentId(key); // Get shmid
    Shmem *shm = attach(shmid);
    int status = get_message(shm, &msg);
    shmdt(shm);  // Detach from memory
    if (status == -1){
      return -1;
    }
    return msg;
}
