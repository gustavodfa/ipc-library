#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include "util.c"

void printQueue(Shmem *shm) {
  for (int i = 0; i < shm->size; i++) {
    printf("%d ", shm->queue[i]);
  }
  printf("\n");
}

void sanitize_members(Shmem *shm) {
  for (int i = 0; i < SHR_MEM_MAX_MEMBERS; i++) {
    Member *m = &shm->members[i];
    if (m->membership_status != NOT_A_MEMBER) {
      if (kill(m->pid, 0) != 0) { // Proccess does not exist. Carefull, pid may have been reused.
        m->last_read=-1;
        m->membership_status = -1;
        m->pid = -1;
        shm->num_members--;
      }
    }
  }
}

int last_read_message(Shmem *shm) {
  sanitize_members(shm);
  int last_read = SHR_MEM_MAX_MESSAGES + 1;
  for (int i = 0; i < SHR_MEM_MAX_MEMBERS; i++) {
    if (shm->members[i].membership_status == WR_MEMBER && shm->members[i].last_read < last_read) {
      last_read = shm->members[i].last_read;
    }
  }
  return last_read;
}

//[1,2,3,4,5,6]
// Shifts an array to the left by an given amount.
void lshift(Shmem *shm, int amount) {
  for (int i = amount; i < shm->size; i++) {
    shm->queue[i - amount] = shm->queue[i];
    shm->queue[i] = -1;
  }
  shm->last_message -= amount;
}

// Cleans shm's queue, removing messages already
// read by all of its members. Returns 0 if
// successful and -1 otherwise.
int sanitize_queue(Shmem *shm) {
  int last_read = last_read_message(shm);
  if (last_read == -1)
    return last_read;
  else if (last_read == SHR_MEM_MAX_MESSAGES + 1) // there is no one to read.
    last_read = 0;

  lshift(shm, last_read + 1);
  
  // Subtract last read for every member.
  for (int i = 0; i < SHR_MEM_MAX_MEMBERS; i++) {
    Member *m = &shm->members[i];
    if (m->membership_status == WR_MEMBER)
      m->last_read -= last_read + 1;
  }
  return last_read + 1;
}

// Full full returns 1 if the buffer is full or 0 if not.
int full(Shmem *shm) {
  if (shm->last_message+1 >= shm->size) {
    return BUFFER_IS_FULL;
  }
  return 0;
}

int haveUnreadMessages(Shmem *shm, int last_read) {
  if (last_read + 1 > shm->last_message){
    return -1;
  }
  return 0;
}

// Returns the current proccess membership.
Member* get_member(Shmem *shm) {
  pid_t pid = getpid();
  for (int i = 0; i < SHR_MEM_MAX_MEMBERS; i++) {
    if (pid == shm->members[i].pid) {
      return &shm->members[i];
    }
  }
  return NULL;
}

// Returns the membership status of the process for the topic.
int get_mship_status(Shmem *shm) {
  pid_t pid = getpid();

  for (int i = 0; i <= SHR_MEM_MAX_MEMBERS; i++) {
    if (pid == shm->members[i].pid) {
      return shm->members[i].membership_status;
    }
  }
  return -1;
}

// join creates a new member or changes membership_status for existing one.
int join(Shmem *shm, int membership_status, int tid) {
  sanitize_members(shm);
  int status = get_mship_status(shm);
  if (status != -1) {
    Member* m = get_member(shm);
    m->membership_status = membership_status;
  } else {
    if (shm->num_members >= SHR_MEM_MAX_MEMBERS){
      return -1;
    }
    for (int i = 0; i < SHR_MEM_MAX_MEMBERS; i++) {
      if (shm->members[i].membership_status == -1) {
        shm->members[i].pid = getpid();
        shm->members[i].membership_status = membership_status;
        shm->members[i].last_read = shm->last_message; // New member can read messages that were already in the buffer.
        shm->num_members ++;
        return 0;
      }  
    }
    return -1;
  }
  return 0;
}

// cancelSubs cancels subscription for the current process.
int cancelSubs(Shmem *shm, int tid) {
  int member_status = get_mship_status(shm);
  if (member_status == NOT_A_MEMBER) {
    fprintf(stderr, "error: process is not a member. Can't cancel subscription.\n");
    return NOT_A_MEMBER;
  }
  Member *m = get_member(shm);
  m->pid = -1;
  m->membership_status = -1;
  m->last_read = -1;
  shm->num_members--;
  
  sanitize_queue(shm);
  if (shm->num_readers_waiting + shm->num_writers_waiting == shm->num_members && shm->num_members != 0){
    // possible deadlock if i leave. Gotta wake someone up.
    if (last_read_message(shm) == shm->last_message){ // every reader has read all messages.
      release(tid, SEM_WRITER, shm->num_writers_waiting);
      shm->num_writers_waiting = 0;
    } else {
        release(tid, SEM_READER, shm->num_readers_waiting);
        shm->num_readers_waiting = 0;
    }
  }
  return 0;
}

// Gets first unread message if available,
// returns NOT_A_MEMBER if the process is not a reader and
// NO_NEW_MESSAGES if inbox is fully read by the process.
int get_message(Shmem *shm, int *msg, int tid) {
  int member_status = get_mship_status(shm);
  if (member_status != WR_MEMBER) {
    fprintf(stderr, "error: process hasn't permissions to access inbox\n");
    return NOT_A_MEMBER;
  }
  Member *m = get_member(shm);
  while (haveUnreadMessages(shm, m->last_read) == -1) {
    release(tid, SEM_MUTEX, 1);
    shm->num_readers_waiting++;
    acquire(tid, SEM_READER);
    acquire(tid, SEM_MUTEX);
  }
  *msg = shm->queue[m->last_read + 1];
  m->last_read++;
  int last_message_all = last_read_message(shm);
  if (last_message_all > 0 && shm->num_writers_waiting > 0) {
    release(tid, SEM_WRITER, shm->num_writers_waiting);
    shm->num_writers_waiting = 0;
  }
  return 0;
}

// add_message adds a message to the queue. Returns NOT_A_MEMBER
// if you are not allowed to write and BUFFER_IS_FULL
// if buffer is full.
// 
int add_message(Shmem *shm, int msg, int tid) {
  int member_status = get_mship_status(shm);
  int erased = 0;
  if (member_status == NOT_A_MEMBER) {
    printf("You are not a member!\n");
    return NOT_A_MEMBER;
  }
  // This shouldn't happen
  while (full(shm) && (erased = sanitize_queue(shm)) == -1){
    release(tid, SEM_MUTEX, 1);
    shm->num_writers_waiting++;
    acquire(tid, SEM_WRITER);
    acquire(tid, SEM_MUTEX);
  }
  shm->last_message++;
  shm->queue[shm->last_message] = msg;
  if (shm->num_readers_waiting > 0) {
    release(tid, SEM_READER, shm->num_readers_waiting);
    shm->num_readers_waiting = 0;
  }
  return 0;
}