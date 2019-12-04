/*
 * Message queueing library for inter-process comunication on Linux systems.
 *
 * @author Gustavo Alves
 * @author Marcos Barros
 *
 */

#include "msgq.h"
#include "string.h"
#include <sys/sem.h>
#include "Shmem.c"

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
  init.last_writer_increment = -1;
  Shmem *shm;
  int status = attachTopicId(topic_id, IPC_CREAT|IPC_EXCL, &shm);
  if (status == -1){
    return -1;
  }
  *shm = init;
  shmdt(shm);
  status = generateSems(topic_id, bufSize);
  if (status == -1) {
    return -1;
  }
  return 1;
}

int pubsub_join(int topic_id) {
  acquire(topic_id, 0);
  Shmem *shm;
  int status = attachTopicId(topic_id, IPC_EXCL, &shm);
  if (status == -1){
    release(topic_id, 0, 1);
    return -1;
  }
  status = join(shm, 0, topic_id);
  shmdt(shm);
  release(topic_id, 0, 1);
  return status;
}

int pubsub_subscribe(int topic_id) {
  acquire(topic_id, 0);
  Shmem *shm;
  int status = attachTopicId(topic_id, IPC_EXCL, &shm);
  if (status == -1){
    release(topic_id, 0, 1);
    return -1;
  }
  status = join(shm, 1, topic_id);
  shmdt(shm);
  release(topic_id, 0, 1);
  return status;
}

int pubsub_cancel(int topic_id) {
  acquire(topic_id, 0);
  Shmem *shm;
  int status = attachTopicId(topic_id, IPC_EXCL, &shm);
  if (status == -1){
    release(topic_id, 0, 1);
    return -1;
  }
  status = cancelSubs(shm, topic_id);
  shmdt(shm);
  release(topic_id, 0, 1);
  return status;
}

// pubsub_publish will write to a topic id and return -1 if action couldn't be performed.
int pubsub_publish(int topic_id, int msg) {
  acquire(topic_id, SEM_MUTEX);
  int status;
  Shmem *shm;
  status = attachTopicId(topic_id, 0, &shm);
  if (status == -1){
    return -1;
  }
  release(topic_id, SEM_MUTEX, 1);

  acquire(topic_id, SEM_WRITER);
  acquire(topic_id, SEM_MUTEX);
  status = add_message(shm, msg, topic_id); //Write
  shmdt(shm);  // Detach from memory
  if (status == -1 || status == -3){
      release(topic_id, SEM_MUTEX, 1);
      release(topic_id, SEM_WRITER, 1);
      return -1;
  }
  release(topic_id, SEM_MUTEX, 1);
  return msg;
}

// pubsub_read will read first unread message from topic id
// and return -1 if no message is found.
int pubsub_read(int topic_id) {
  int msg, status;
  Shmem *shm;
  acquire(topic_id, SEM_MUTEX);
  status = attachTopicId(topic_id, 0, &shm);
  if (status < 0){
    return -1;
  }
  // Discover my sem_id
  int sem_num = get_sem_num(shm);
  release(topic_id, SEM_MUTEX, 1);

  acquire(topic_id, sem_num);
  acquire(topic_id, SEM_MUTEX);
  status = get_message(shm, &msg, topic_id);
  shmdt(shm);  // Detach from memory
  if (status == -1){
    release(topic_id, SEM_MUTEX, 1);
    return -1;
  }
  release(topic_id, SEM_MUTEX, 1);
  return msg;
}
