#include <time.h>
#include <stdio.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>

#define QUEUE_MAX_SIZE 128

#define SIG_WAKEUP  SIGUSR1
#define SIG_INVITE  SIGUSR2

#define SEM_WAITING_ROOM_ID 0
#define SEM_CHAIR_ID 1
#define SEM_SHAVING_ID 2
#define SEM_SHAVING_ROOM_ID 3
#define MEM_BARBERSHOP_ID 4

struct queue {
    int max_size;
    int clients;
    pid_t clients_queue[QUEUE_MAX_SIZE];
    int front;
    int back;
};


struct barbershop {

    struct queue clients_queue;
    int empty_sits_number;

    pid_t barber_pid;
    pid_t shaved_client_pid;
    int is_sleeping;

};

void init_queue(struct queue *q, int size) {

    q -> max_size = size;
    q -> clients = 0;
    q -> front = 0;
    q -> back = 1;
}

int enqueue(struct queue *q, pid_t client_pid) {

    if(q -> clients >= q -> max_size)
        return -1;

    int position = (q -> back - 1) % q -> max_size;
    q -> clients_queue[position] = client_pid;
    q -> back = position;
    q -> clients++;

    return 0;
}

pid_t dequeue(struct queue *q) {

    if(q -> clients == 0)
        return -1;

    pid_t result = q -> clients_queue[q -> front];
    q -> front = (q -> front - 1) % q -> max_size;
    q -> clients--;

    return result;
}

void print_msg(const char *message) {

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    fprintf(stdout, "Time: %ld.%06ld\tPid %d:\t %s", ts.tv_sec, ts.tv_nsec / 1000, getpid(), message);
    fflush(stdout);
}

int decrase_sem(int sem_id) {

    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;

    return semop(sem_id, &buf, 1);
}

int increase_sem(int sem_id) {

    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;

    return semop(sem_id, &buf, 1);
}

int wait_sem(int sem_id) {

    struct sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 0;
    buf.sem_flg = 0;

    return semop(sem_id, &buf, 1);
}
