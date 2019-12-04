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
  int last_read;
  int membership_status; // change attribute name to membership_status
  int sem_num;
} Member;

typedef struct Shmem {
    int size;
    int last_message;
    int last_writer_increment;
    int num_members;
    int queue[SHR_MEM_MAX_MESSAGES];
    Member members[SHR_MEM_MAX_MEMBERS];
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
  int semid = semget(key, 2 + SHR_MEM_MAX_MEMBERS, options | 0777);
  if (semid == -1) {
    perror("semget");
    return -1;
  }
  return semid;
}

int resetReaderSem(int tid, int sem_num, int init_num) {
  key_t key = generateKey(tid);
  if (key == -1) {
    return -1;
  }
  int semid = semaphoreId(key, 0666);
  if (semid == -1) {
    return -1;
  }
  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
  } arg_ctl ;

  arg_ctl.val = init_num;
  if (semctl(semid, sem_num, SETVAL, arg_ctl) == -1) {
    perror("Erro inicializacao semaforo") ;
  return -1;
  }
  return 0;
}

int getSemValue(int sem_num, int tid) {
  key_t key = generateKey(tid);
  if (key == -1) {
    return -1;
  }
  int semid = semaphoreId(key, 0666);
  if (semid == -1) {
    return -1;
  }
  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int array[1];
  } arg_ctl ;
  return semctl(semid, sem_num, GETVAL, arg_ctl);
}

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
  arg_ctl.val = bufSize;
  if (semctl(semid,1,SETVAL,arg_ctl) == -1) {
    perror("Erro inicializacao semaforo") ;
    return -1;
  }
  return 0;
}

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
  sempar[0].sem_flg = (sem_num == SEM_MUTEX) ? SEM_UNDO : 0;
  if (semop(semid, sempar, 1) == -1){
    perror("Erro operacao P");
    return -1;
  }
  return 0;
}

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
    perror("Erro operacao V");
    return -1;
  }
  return 0;
}