#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <zconf.h>

#include "common.h"

int shared_memory_id;
int semaphore_id;

void signal_handler(int signal) {
    printf("Aborting...\n");
    exit(0);
}

void invite_client() {
    pid_t client_pid = barber_shop->queue[0];
    barber_shop->current_client = client_pid;
    printf("Invited client %i\t\tTime: %lo\n", client_pid, get_time());

    for (int i = 0; i < barber_shop->clients - 1; ++i) {
        barber_shop->queue[i] = barber_shop->queue[i + 1];
    }

    barber_shop->queue[barber_shop->clients - 1] = 0;
    barber_shop->clients -= 1;
}

void serve_client() {
    printf("Started shaving client %i\tTime: %lo\n", barber_shop->current_client, get_time());
    sleep(1);
    printf("Finished shaving client %i\tTime: %lo\n", barber_shop->current_client, get_time());

    barber_shop->current_client = 0;
}

void clean_up() {
    if (semaphore_id != 0) {
        semctl(semaphore_id, 0, IPC_RMID);
    }
    if (shared_memory_id != 0) {
        shmctl(shared_memory_id, IPC_RMID, NULL);
    }
}

void init(int argc, char **argv) {
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    atexit(clean_up);

    if (argc < 2) ext("Not enough arguments!\n");

    int waiting_room_size = (int) strtol(argv[1], 0, 10);
    if (waiting_room_size > MAX_QUEUE_SIZE) {
        char buffer[50];
        sprintf(buffer, "Waiting room size has to be less than %i \n", MAX_QUEUE_SIZE);
        ext(buffer);
    }
    key_t project_key = ftok(PATH, ID);

    if (project_key == -1)
        ext("Barber - key\n");

    shared_memory_id = shmget(project_key, sizeof(struct Barber_shop), S_IRWXU | IPC_CREAT);

    if (shared_memory_id == -1)
        ext("Barber - shmget\n");

    barber_shop = shmat(shared_memory_id, 0, 0);
    if (barber_shop == (void *) -1)
        ext("Barber - shmat\n");

    semaphore_id = semget(project_key, 1, IPC_CREAT | S_IRWXU);
    if (semaphore_id == -1)
        ext("Barber - semget\n");

    semctl(semaphore_id, 0, SETVAL, 0);

    barber_shop->barber_status = BARBER_SLEEPING;
    barber_shop->clients = 0;
    barber_shop->queue_size = waiting_room_size;
    barber_shop->current_client = 0;

    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
        barber_shop->queue[i] = 0;
}

int main(int argc, char **argv) {
    init(argc, argv);

    while (1) {
        get_sem(semaphore_id);
        switch (barber_shop->barber_status) {
            case BARBER_IDLE:
                if (is_empty()) {
                    printf("Barber fell asleep\t\tTime: %lo\n", get_time());
                    barber_shop->barber_status = BARBER_SLEEPING;
                } else {
                    invite_client();
                    barber_shop->barber_status = BARBER_BUSY;
                }
                break;
            case BARBER_AWOKEN:
                printf("Woke up\t\t\t\tTime: %lo\n", get_time());
                if (barber_shop->current_client != 0) {
                    barber_shop->barber_status = BARBER_BUSY;
                    break;
                }
                barber_shop->barber_status = BARBER_IDLE;
                break;
            case BARBER_BUSY:
                serve_client();
                barber_shop->barber_status = BARBER_IDLE;
                break;
            default:
                break;
        }
        release_semaphore(semaphore_id);
    }
}