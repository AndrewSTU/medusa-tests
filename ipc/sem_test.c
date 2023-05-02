#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <pthread.h>
#include <string.h>

int silent_mode = 0;
int buffer_size = 20;

int stop_producing = 0;
int stop_consuming = 25;

float sleep_consumer = 1;
float sleep_producer = 1;

int num_producers = 5;
int num_consumers = 5;

int mutex_id;
int full_id;
int empty_id;

void sem_wait(int sem_id) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;

    semop(sem_id, &sem_op, 1);
}

void sem_signal(int sem_id) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;

    semop(sem_id, &sem_op, 1);
}

void release_producers() {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = num_producers;
    sem_op.sem_flg = 0;

    semop(empty_id, &sem_op, 1);
}

void *consumer(void* id) {
    int consumer_id = *(int*) id;
    int consumed = 0;

    while (consumed < stop_consuming){
        sem_wait(full_id);
        sem_wait(mutex_id);

        consumed += 1;
        if (!silent_mode) {
            printf("Consumer %d:\tConsumed item.\n", consumer_id);
        }

        sem_signal(mutex_id);
        sem_signal(empty_id);

        sleep(sleep_consumer);
    }
    pthread_exit(NULL);
}

void *producer(void* id) {
    int producer_id = *(int*) id;

    while (!stop_producing) {
        sem_wait(empty_id);
        sem_wait(mutex_id);

        if (!silent_mode) {
            printf("Producer %d:\tProduced item.\n", producer_id);
        }

        sem_signal(mutex_id);
        sem_signal(full_id);

        sleep(sleep_producer);
    }
}


void print_help() {
    printf("Usage: ./program [OPTIONS]\n");
    printf("Options:\n");
    printf("\t-s,   --silent                  Do not print any output\n");
    printf("\t-bs,  --buffer-size N           Buffer size (default: 20)\n");
    printf("\t-c,   --consumers N             Number of consumer threads (default: 5)\n");
    printf("\t-p,   --producers N             Number of producer threads (default: 5)\n");
    printf("\t-cl,  --consumer-limit N        Consumer consumption stop limit (default: 25)\n");
    printf("\t-cs,  --consumer-sleep TIME     Consumer sleep time in seconds (default: 1)\n");
    printf("\t-ps,  --producer-sleep TIME     Producer sleep time in seconds (default: 1)\n");
    printf("\t-h,   --help                    Display this help and exit\n");
}

int main(int argc, char *argv[]) {

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0) {
            silent_mode = 1;
        } else if (strcmp(argv[i], "-bs") == 0 || strcmp(argv[i], "--buffer-size") == 0) {
            buffer_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--consumers") == 0) {
            num_consumers = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--producers") == 0) {
            num_producers = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-cl") == 0 || strcmp(argv[i], "--consumer-limit") == 0) {
            stop_consuming = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-cs") == 0 || strcmp(argv[i], "--consumer-sleep") == 0) {
            sleep_consumer = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ps") == 0 || strcmp(argv[i], "--producer-sleep") == 0) {
            sleep_producer = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            exit(0);
        } else {
            printf("Unknown option: %s\n", argv[i]);
            exit(1);
        }
    }

    // Create semaphores
    mutex_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    full_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    empty_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);

    // Initialize semaphores
    semctl(mutex_id, 0, SETVAL, 1);
    semctl(full_id, 0, SETVAL, 0);
    semctl(empty_id, 0, SETVAL, buffer_size);

    // Create instances
    pthread_t producers[num_producers];
    pthread_t consumers[num_consumers];

    int producer_ids[num_producers];
    int consumer_ids[num_consumers];

    // Create producers
    for (int i = 0; i < num_producers; i++) {
        producer_ids[i] = i;
        pthread_create(&producers[i], NULL, producer, &producer_ids[i]);
    }

    // Create consumers
    for (int i = 0; i < num_consumers; i++) {
        consumer_ids[i] = i;
        pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]);
    }

    // Join consumers
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }

    if (!silent_mode) {
        printf("Consumers are done.\n");
    }
    stop_producing = 1;
    release_producers();

    // Join producers
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }

    if (!silent_mode) {
        printf("All done.\n");
    }

    // Delete semaphores
    semctl(mutex_id, 0, IPC_RMID);
    semctl(full_id, 0, IPC_RMID);
    semctl(empty_id, 0, IPC_RMID);

    if (!silent_mode) {
        printf("Semaphores deleted.\n");
    }
    return 0;
}
