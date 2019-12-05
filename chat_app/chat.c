#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "pthread.h"
#include "../src/msgq.c"

#define OPTION 1
#define TOPIC_ID 2
#define BUFFER_SIZE 8

void check_arg(int argc, char const *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "usage: chat [-c/-j/--create/--join] [TOPIC_ID]\n");
    exit(1);
  }

  if (strcmp(argv[OPTION], "-c") && strcmp(argv[OPTION], "--create") &&
      strcmp(argv[OPTION], "-j") && strcmp(argv[OPTION], "--join")) {
      fprintf(stderr, "error: unknown option!\nusage: chat [-c/-j/--create/--join] [TOPIC_ID]\n");
      exit(1);
  }
}

void *read_topic(void *arg) {
  int topic_id = *((int*) arg);
  int value;

  for (;;) {
    value = pubsub_read(topic_id);
    printf("got: %d\n", value);
  }
}

int speak_to_topic(int topic_id) {
  char input[8];
  int value;
  for (;;) {
    scanf("%s", input);
    if (strcmp(input, "/exit") == 0) {
      pubsub_cancel(topic_id);
      exit(0);
    }
    value = atoi(input);
    pubsub_publish(topic_id, value);
  }
}

void join_topic(int topic_id) {
  pthread_t reading_thread;

  if (pubsub_subscribe(topic_id) == -1) {
    fprintf(stderr, "error: couldn't join topic!\n");
    exit(1);
  }
  pthread_create(&reading_thread, NULL, read_topic, &topic_id);
  speak_to_topic(topic_id);
}

void create_topic(int topic_id) {
  if (pubsub_create_topic(topic_id, BUFFER_SIZE) == -1) {
    fprintf(stderr, "error: couldn't create topic!\n");
    exit(1);
  }
  join_topic(topic_id);
}

int main(int argc, char const *argv[])
{
  int topic_id;

  check_arg(argc, argv);
  topic_id = atoi(argv[TOPIC_ID]);
  pubsub_init();
  if (!(strcmp(argv[OPTION], "-c") && strcmp(argv[OPTION], "--create")))
    create_topic(topic_id);
  else if (!(strcmp(argv[OPTION], "-j") && strcmp(argv[OPTION], "--join")))
    join_topic(topic_id);

  return 0;
}
