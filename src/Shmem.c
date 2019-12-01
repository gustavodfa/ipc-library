#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define SHR_MEM_FILE "/tmp/shrmem"

typedef struct Shmem {
    int size;
    int lastMessage;
    int queue[100];
} Shmem;



int full(Shmem *shm) {
  if (shm->lastMessage >= shm->size) {
    return 1;
  }
  return 0;
}

int get_message(Shmem *shm, int *msg) {
  if (full(shm)) {
      return -1;
  }
  *msg = shm->queue[shm->lastMessage];
  return 0;
}

int add_message(Shmem *shm, int msg) {
  if (full(shm)) {
      return -1;
  }
  shm->lastMessage++;
  shm->queue[shm->lastMessage] = msg;
  return 0;
}