#include <sys/sem.h>

// SHARED MEM INIT CONSTANTS
#define SHR_MEM_FILE "/tmp/shrmem"
#define SHR_MEM_MAX_MEMBERS 50
#define SHR_MEM_MAX_MESSAGES 100

// MEMBER STATUS
#define WR_MEMBER 1
#define WONLY_MEMBER 0
#define NOT_A_MEMBER -1

// DEPRECATED
#define NO_NEW_MESSAGES -2
#define BUFFER_IS_FULL -3

// MUTEXES
#define SEM_MUTEX 0
#define SEM_WRITER 1
#define SEM_READER 2


// SHARED MEM STRUCTS
typedef struct Member {
  pid_t pid;
  int last_read; // last message a member has read.
  int membership_status; // membership_status (WR, W, NOT_MEMBER)
} Member;

typedef struct Shmem {
    int size; // Shared memory size, max == SHR_MEM_MAX_MESSAGES
    int last_message; // last written message index.
    int num_readers_waiting; // number of readers waiting in the readers sem
    int num_writers_waiting; // number of writers waiting in the writers sem
    int num_members; // num of members of the shmem
    int queue[SHR_MEM_MAX_MESSAGES]; // message queue
    Member members[SHR_MEM_MAX_MEMBERS]; // members array
} Shmem;

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

// segmentId will generate the shared memory segment id for key and options.
int segmentId(key_t key, int options) {
  int shmid;
  shmid = shmget(key, sizeof(Shmem), 0777 | options);
  if (shmid == -1) {
    perror("shmget");
    return -1;
  }
  return shmid;
}

// attach calls shmat and attachs shared memory to pointer dst.
int attach(int shmid, Shmem **dst){
  void *shm = shmat(shmid, (void*)0, 0); // Attach to memory
  if (shm == (void *)(-1)) {
    perror("shmat");
    return -1;
  }
  *dst = shm;
  return 0;
}

// attachTopicId 
// takes a topic id, options and *shm pointer
// will try to attach to the shared memory identified by the topic_id
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

// sempahoreId returns the semaphore id for key and options.
int semaphoreId(key_t key, int options) {
  int semid = semget(key, 3, options | 0777);
  if (semid == -1) {
    perror("semget");
    return -1;
  }
  return semid;
}

// semaphoreInit will initialize three semaphores, one acting as a mutex and other two as a cond var.
int semaphoreInit(int semid, int bufSize) {
  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
  } arg_ctl ;

  // Mutex.
  arg_ctl.val = 1;
  if (semctl(semid,0,SETVAL,arg_ctl) == -1) {
    perror("Erro inicializacao semaforo") ;
    return -1;
  }

  // Writer.
  arg_ctl.val = 0;
  if (semctl(semid,1,SETVAL,arg_ctl) == -1) {
    perror("Erro inicializacao semaforo") ;
    return -1;
  }

  // Reader.
  arg_ctl.val = 0;
  if (semctl(semid,1,SETVAL,arg_ctl) == -1) {
    perror("Erro inicializacao semaforo") ;
    return -1;
  }
  return 0;
}

// generateSems is the init function for semaphore generation.
int generateSems(int topicId, int bufSize) {
  key_t key = generateKey(topicId);
  if (key == -1) {
    return -1;
  }
  int semid = semaphoreId(key, IPC_CREAT|IPC_EXCL|0666);
  if (semid == -1) {
    return -1;
  }
  return semaphoreInit(semid, bufSize);
}

// Acquire will try to acquire semaphore with sem_num.
int acquire(int topicId, int sem_num) {
  key_t key = generateKey(topicId);
  if (key == -1) {
    return -1;
  }
  int semid = semaphoreId(key, 0);
  if (semid == -1) {
    return -1;
  }
  struct sembuf sempar[1];
  sempar[0].sem_num = sem_num;
  sempar[0].sem_op = -1;
  sempar[0].sem_flg = SEM_UNDO;
  if (semop(semid, sempar, 1) == -1){
    perror("Erro operacao acquire");
    return -1;
  }
  return 0;
}

// release will try to release semaphore for release_value.
int release(int topicId, int sem_num, int release_value)
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
  sempar[0].sem_num = sem_num;
  sempar[0].sem_op =  release_value;
  sempar[0].sem_flg = SEM_UNDO;
  if (semop(semid, sempar, 1) == -1) {
    perror("Erro operacao release");
    return -1;
  }
  return 0;
}