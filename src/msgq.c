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
#include <sys/sem.h>

// KEY GENERATION
key_t generateKey(int topicId) {
    key_t key = ftok(SHR_MEM_FILE, topicId);
    if (key == -1) {
        perror("ftok error");
        return -1;
    }
    return key;
}

// SHARED MEMORY
int segmentId(key_t key, int options) {
  int shmid;
  shmid = shmget(key, sizeof(Shmem), 0777 | options);
  if (shmid == -1) {
    perror("shmget");
    return -1;
  }
  return shmid;
}

int attach(int shmid, Shmem **dst){
  void *shm = shmat(shmid, (void*)0, 0); // Attach to memory
  if (shm == (void *)(-1)) {
    perror("shmat");
    return -1;
  }
  *dst = shm;
  return 0;
}

int attachTopicId(int topic_id, int options,Shmem **shm){
  key_t key = generateKey(topic_id); // Get key for the topic.
  if (key == -1) {
    return -1;
  }
  int shmid = segmentId(key, options); // Try to get a segment id, if already exists, delete it.
  if (shmid == -1) {
    return -1;
  }
  int status = attach(shmid, shm); // Attach to memory
  return status;
}

// SEMAPHORES

int semaphoreId(key_t key, int options) {
  int semid = semget(key, 1, options | 0777);
  if (semid == -1) {
    perror("semget");
    return -1;
  }
  return semid;
}

int semaphoreInit(int semid, int initValue) {
  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
  } arg_ctl ;

  arg_ctl.val = initValue;
  if (semctl(semid,0,SETVAL,arg_ctl) == -1) {
    perror("Erro inicializacao semaforo") ;
    return -1;
  }
  return 0;
}

int generateSems(int topicId, int initValue) {
  key_t key = generateKey(topicId);
  if (key == -1) {
    return -1;
  }
  int semid = semaphoreId(key, IPC_CREAT|IPC_EXCL|0666);
  if (semid == -1) {
    return -1;
  }
  return semaphoreInit(semid, initValue);
}

int acquire(int topicId) {
  key_t key = generateKey(topicId);
  if (key == -1) {
    return -1;
  }
  int semid = semaphoreId(key, 0);
  if (semid == -1) {
    return -1;
  }
  struct sembuf sempar[1];
  sempar[0].sem_num = 0;
  sempar[0].sem_op = -1;
  sempar[0].sem_flg = SEM_UNDO;
  if (semop(semid, sempar, 1) == -1){
    perror("Erro operacao P");
    return -1;
  }
  return 0;
}

int release(int topicId)
{
  key_t key = generateKey(topicId);
  if (key == -1) {
    return -1;
  }
  int semid = semaphoreId(key, 0);
  if (semid == -1) {
    return -1;
  }
  struct sembuf sempar[1];
  sempar[0].sem_num = 0;
  sempar[0].sem_op =  1;
  sempar[0].sem_flg = SEM_UNDO;
  if (semop(semid, sempar, 1) == -1) {
    perror("Erro operacao V");
    return -1;
  }
  return 0;
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
  init.last_message = -1;
  init.size = bufSize;
  memset(init.queue, 0, sizeof(init.queue));
  init.num_members = -1;
  for (int i = 0; i < SHR_MEM_MAX_MEMBERS; i++) {
    init.members[i].membership_status = -1;
  }
  Shmem *shm;
  int status = attachTopicId(topic_id, IPC_CREAT|IPC_EXCL, &shm);
  if (status == -1){
    return -1;
  }
  *shm = init;
  shmdt(shm);
  status = generateSems(topic_id, 1);
  if (status == -1) {
    return -1;
  }
  return 1;
}

int pubsub_join(int topic_id) {
  acquire(topic_id);
  Shmem *shm;
  int status = attachTopicId(topic_id, IPC_EXCL, &shm);
  if (status == -1){
    release(topic_id);
    return -1;
  }
  status = join(shm, 0);
  shmdt(shm);
  release(topic_id);
  return status;
}

int pubsub_subscribe(int topic_id) {
  acquire(topic_id);
  Shmem *shm;
  int status = attachTopicId(topic_id, IPC_EXCL, &shm);
  if (status == -1){
    release(topic_id);
    return -1;
  }
  status = join(shm, 1);
  shmdt(shm);
  release(topic_id);
  return status;
}

int pubsub_cancel(int topic_id) {
  acquire(topic_id);
  Shmem *shm;
  int status = attachTopicId(topic_id, IPC_EXCL, &shm);
  if (status == -1){
    release(topic_id);
    return -1;
  }
  status = cancelSubs(shm);
  shmdt(shm);
  release(topic_id);
  return status;
}

// pubsub_publish will write to a topic id and return -1 if action couldn't be performed.
int pubsub_publish(int topic_id, int msg) {
  acquire(topic_id);
  int status;
  Shmem *shm;
  status = attachTopicId(topic_id, 0, &shm);
  if (status == -1){
    release(topic_id);
    return -1;
  }
  status = add_message(shm, msg); //Write
  shmdt(shm);  // Detach from memory
  if (status < -1){
    release(topic_id);
    return -1;
  }
  release(topic_id);
  return msg;
}

// pubsub_read will read first unread message from topic id
// and return -1 if no message is found.
int pubsub_read(int topic_id) {
  acquire(topic_id);
  int msg, status;
  Shmem *shm;
  status = attachTopicId(topic_id, 0, &shm);
  if (status < 0){
    release(topic_id);
    return -1;
  }
  status = get_message(shm, &msg);
  shmdt(shm);  // Detach from memory
  if (status < 0){
    release(topic_id);
    return -1;
  }
  release(topic_id);
  return msg;
}
