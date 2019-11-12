int pubsub_init(void);
int pubsub_create_topic(int topic_id);
int pubsub_join(int topic_id);
int pubsub_subscribe(int topic_id);
int pubsub_cancel(int topic_id);
int pubsub_publish(int topic_id, int msg);
int pubsub_read(int topic_id);
