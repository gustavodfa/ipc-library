#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define SHR_MEM_FILE "/tmp/shrmem"
#define SHR_MEM_MAX_MEMBERS 50
#define SHR_MEM_MAX_MESSAGES 100
#define WR_MEMBER 1
#define WONLY_MEMBER 0
#define NOT_A_MEMBER -1
#define NO_NEW_MESSAGES 1
#define BUFFER_IS_FULL 2

typedef struct Member {
  pid_t pid;
  int last_read;
  int membership_status; // change attribute name to membership_status
} Member;

typedef struct Shmem {
    int size;
    int last_message;
    int last_member;
    int queue[SHR_MEM_MAX_MESSAGES];
    Member members[SHR_MEM_MAX_MEMBERS];
} Shmem;

//[1,2,3,4,5,6]
// Shifts an array to the left by an given amount.
void lshift(Shmem *shm, int amount) {
  for (int i = amount; i < SHR_MEM_MAX_MESSAGES; i++) {
    shm->queue[i - amount] = shm->queue[i];
    shm->queue[i] = -1;
  }
  shm->last_message -= amount;
}

// Cleans shm's queue, removing messages already
// read by all of its members. Returns 0 if
// successful and -1 otherwise.
int sanitize_queue(Shmem *shm) {
  int last_read_message = SHR_MEM_MAX_MESSAGES;

  for (int i = 0; i < SHR_MEM_MAX_MEMBERS; i++) {
    Member *m = &shm->members[i];
    if (m->membership_status == WR_MEMBER && m->last_read < SHR_MEM_MAX_MESSAGES)
      last_read_message = m->last_read;
  }
  if (last_read_message == -1)
    return -1;
  lshift(shm, last_read_message + 1);
  return 0;
}

// Full full returns 1 if the buffer is full or 0 if not.
int full(Shmem *shm) {
  if (shm->last_message >= shm->size) {
    return BUFFER_IS_FULL;
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

// Returns the membership status of the process for the topic.
int get_mship_status(Shmem *shm) {
  pid_t pid = getpid();

  for (int i = 0; i <= shm->last_member; i++) {
    if (pid == shm->members[i].pid) {
      return shm->members[i].membership_status;
    }
  }
  return -1;
}


// join creates a new member or turns existing one to a reader.
int join(Shmem *shm, int membership_status) {
  int status = get_mship_status(shm);
  if (status != -1) {
    Member* m = get_member(shm);
    m->membership_status = membership_status;
  } else {
    Member new;
    new.pid = getpid();
    new.last_read = -1;
    new.membership_status = membership_status;

    if (shm->last_member >= SHR_MEM_MAX_MEMBERS){
      return -1;
    }
    shm->last_member++;
    shm->members[shm->last_member] = new;
  }
  return 0;
}

// Gets first unread message if available,
// returns NOT_A_MEMBER if the process is not a reader and
// NO_NEW_MESSAGES if inbox is fully read by the process.
int get_message(Shmem *shm, int *msg) {
  Member *m = get_member(shm);
  int member_status = m->membership_status;

  if (member_status != WR_MEMBER) {
    fprintf(stderr, "error: process hasn't permissions to access inbox\n");
    return NOT_A_MEMBER;
  }
  if (m->last_read + 1 >= SHR_MEM_MAX_MESSAGES) {
    fprintf(stderr, "error: there are no new messages\n");
    return NO_NEW_MESSAGES;
  }
  *msg = shm->queue[m->last_read + 1];
  m->last_read++;
  return 0;
}

// add_message adds a message to the queue. Returns NOT_A_MEMBER
// if you are not allowed to write and BUFFER_IS_FULL
// if buffer is full.
int add_message(Shmem *shm, int msg) {
  int member_status = get_mship_status(shm);

  if (member_status == NOT_A_MEMBER) {
    printf("You are not a member!\n");
    return NOT_A_MEMBER;
  }
  if (full(shm) && sanitize_queue(shm) == -1)
    return BUFFER_IS_FULL;
  shm->last_message++;
  shm->queue[shm->last_message] = msg;
  return 0;
}