#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "../src/msgq.h"

#define CREATE_ROOM 1
#define JOIN_ROOM 2
#define BUFFER_SIZE 10
#define SEND_MESSAGE 1
#define READ_MESSAGE 2
#define EXIT 3

void read_messages(int room) {
  int message;
  printf(":: INBOX ::\n");
  for (;;) {
    message = pubsub_read(room);
    if (message != -1)
      printf("%d\n", message);
    else
      break;
  }
  printf(":::::::::::\n");
}

void chat(int room) {
  int command;
  for (;;) {
    printf("options: [1]send, [2]read and [3]exit\n> ");
    scanf(" %d", &command);

    if (command == EXIT) {
      pubsub_cancel(room);
      break;
    
    } else if (command == SEND_MESSAGE) {
      int message;
      scanf("%d", &message);
      printf("msg escrita: %d\n", message);
      pubsub_publish(room, message);

    } else if (command == READ_MESSAGE) {
      read_messages(room);
    
    } else {
      fprintf(stderr, "error: invalid command\n");
    }

  }
}

int main(void) {
  int option;
  int room;

  printf("Welcome back. Do you want to [1]create or [2]join a room?\n> ");
  scanf("%d", &option);

  if (option == CREATE_ROOM) {
    room = (int) getpid();
    pubsub_init();
    if (pubsub_create_topic(room, BUFFER_SIZE) == -1) {    
      fprintf(stderr, "error: couldn't create topic\n");
      return -1;
    }
    printf("room id: %d\n", room);
    pubsub_join(room);

  } else if (option == JOIN_ROOM) {
    printf("Which room would you like to join?\n");
    scanf(" %d", &room);
    if (pubsub_join(room) == -1) {
      fprintf(stderr, "error: couldn't join room\n");
    }
  }
  chat(room);
  return 0;  
}