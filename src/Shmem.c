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

int deadLockPrevention(Shmem *shm, int tid) {
  int pid = getpid();
  if (getSemValue(SEM_WRITER, tid) >= 0) {
    return 0;
  } // If writers are blocked.
  for (int i = 0; i < SHR_MEM_MAX_MEMBERS; i++) {
    Member *m = &shm->members[i];
    if (m->membership_status != WR_MEMBER || pid == m->pid) {
      continue;
    }
    if (getSemValue(m->sem_num, tid) > 0) {
      return 0;
    }
  } // If all readers are blocked
  shm->last_writer_increment = last_read_message(shm);;
  // Need to wake up Writer to finish job.
  release(tid, SEM_WRITER, 1);
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
  if (last_read == SHR_MEM_MAX_MESSAGES + 1)
    lshift(shm, 1);
  else 
    lshift(shm, last_read + 1);
  // Subtract last read for every member.
  for (int i = 0; i < SHR_MEM_MAX_MEMBERS; i++) {
    Member *m = &shm->members[i];
    if (m->membership_status == WR_MEMBER)
      m->last_read -= last_read + 1;
  }
  return (last_read == SHR_MEM_MAX_MESSAGES + 1) ? last_read : last_read + 1;
}

// Full full returns 1 if the buffer is full or 0 if not.
int full(Shmem *shm) {
  if (shm->last_message+1 >= shm->size) {
    return BUFFER_IS_FULL;
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

int get_sem_num(Shmem *shm) {
  int status = get_mship_status(shm);
  if (status != WR_MEMBER) {
    return -1;
  }
  return get_member(shm)->sem_num;
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
        shm->members[i].last_read = -1; // New member can read messages that were already in the buffer.
        shm->members[i].sem_num = 2 + i;
        resetReaderSem(tid, 2 + i, shm->last_message+1);
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
  deadLockPrevention(shm, tid);
  Member *m = get_member(shm);
  m->pid = -1;
  m->membership_status = -1;
  m->last_read = -1;
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
  if (m->last_read + 1 > shm->last_message) {
    // This should be an error in logic.
    fprintf(stderr, "error: there are no new messages\n");
    return NO_NEW_MESSAGES;
  }
  *msg = shm->queue[m->last_read + 1];
  m->last_read++;
  int last_message_all = last_read_message(shm);
  if (last_message_all >= 0 && shm->last_writer_increment < last_message_all) {
    release(tid, SEM_WRITER, 1); // Release Writers for n that were already readen by all.
    shm->last_writer_increment = last_message_all;
  }
  return 0;
}

int wakeReaders(Shmem *shm, int tid) {
  for (int i = 0; i < SHR_MEM_MAX_MEMBERS; i++) {
    if (shm->members[i].membership_status == WR_MEMBER) {
      release(tid, shm->members[i].sem_num, 1);
    }
  }
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
  if (full(shm) && (erased = sanitize_queue(shm)) == -1)
    return BUFFER_IS_FULL;
  shm->last_message++;
  shm->queue[shm->last_message] = msg;
  if (erased != SHR_MEM_MAX_MESSAGES + 1) { // Someone did read and is alive
    shm->last_writer_increment -= erased;
    wakeReaders(shm, tid);
  }
  return 0;
}