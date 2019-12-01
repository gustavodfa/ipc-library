#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define SHR_MEM_FILE "/tmp/shrmem"
#define SHR_MEM_MAX_MEMBERS 50

typedef struct Member {
  pid_t pid;
  int last_read;
  int is_reader;
} Member;

typedef struct Shmem {
    int size;
    int last_message;
    int last_member;
    int queue[100];
    Member members[50];
} Shmem;

// Full full returns 1 if the buffer is full or 0 if not.
int full(Shmem *shm) {
  if (shm->last_message >= shm->size) {
    return 1;
  }
  return 0;
}

// Returns the current proccess membership.
Member* get_member(Shmem *shm) {
  pid_t pid = getpid();
  for (int i = 0; i < shm->last_member; i++) {
    if (pid == shm->members[i].pid) {
      return &shm->members[i];
    }
  }
  return NULL;
}

// Returns the member status of the process for the topic.
// -1 is not a member.
// 0 is a writter member
// 1 is a reader and writter member.
int is_member(Shmem *shm) {
  pid_t pid = getpid();
  for (int i = 0; i <= shm->last_member; i++) {
    if (pid == shm->members[i].pid) {
      return shm->members[i].is_reader;
    }
  }
  return -1;
}


// join creates a new member or turns existing one to a reader.
int join(Shmem *shm, int is_reader) {
  int status = is_member(shm);
  if (status != -1) {
    Member* m = get_member(shm);
    m->is_reader = is_reader;
  } else {
    Member new;
    new.pid = getpid();
    new.last_read = -1;
    new.is_reader = is_reader;

    if (shm->last_member >= SHR_MEM_MAX_MEMBERS){
      return -1;
    }
    shm->last_member++;
    shm->members[shm->last_member] = new;
  }
  return 0;
}

// get_message gets lattest message in queue. Returns -1 if you are not a reader.
int get_message(Shmem *shm, int *msg) {
  int member_status = is_member(shm);
  if (member_status != 1) {
    printf("You are not a reader!\n");
    return -1;
  }
  *msg = shm->queue[shm->last_message];
  return 0;
}

// add_message adds a message to the queue. Returns -1 if you are not allowed to write.
int add_message(Shmem *shm, int msg) {
  int member_status = is_member(shm);
  if (member_status == -1) {
    printf("You are not a member!\n");
    return -1;
  }
  if (full(shm)) {
      return -1;
  }
  shm->last_message++;
  shm->queue[shm->last_message] = msg;
  return 0;
}