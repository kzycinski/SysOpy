#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <zconf.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>

#include "common.h"

int client_status, shared_memory_id, semaphore_id;

void init() {
    key_t project_key = ftok(PATH, ID);
    if (project_key == -1)
        ext("Client - key\n");

    shared_memory_id = shmget(project_key, sizeof(struct Barber_shop), 0);
    if (shared_memory_id == -1)
        ext("Client - shmget\n");

    barber_shop = shmat(shared_memory_id, 0, 0);
    if (barber_shop == (void *) -1)
        ext("Client - shmat\n");

    semaphore_id = semget(project_key, 0, 0);
    if (semaphore_id == -1)
        ext("Client - semget\n");
}

void run_client(int cuts) {
    pid_t pid = getpid();
    for (int j = 0; j < cuts; ++j) {

        client_status = CLIENT_NEW;

        get_sem(semaphore_id);
        if (barber_shop->barber_status == BARBER_SLEEPING) {
            if (barber_shop->current_client != 0)
                ext("Barber - selected client while sleeping\n");

            printf("Client %i: woke up the barber\t\tTime: %lo\n", pid, get_time());
            barber_shop->barber_status = BARBER_AWOKEN;

            barber_shop->current_client = pid;

        } else if (!is_full()) {
            enter_queue(pid);
            printf("Client %i: entering the queue\t\tTime: %lo\n", pid, get_time());
        } else {
            printf("Client %i: queue full, client leaves\t\tTime: %lo\n", pid, get_time());
            release_semaphore(semaphore_id);
            exit(0);
        }
        release_semaphore(semaphore_id);

        while (client_status == CLIENT_NEW) {
            get_sem(semaphore_id);
            if (barber_shop->current_client == pid) {
                client_status = CLIENT_SITTING;
                printf("Client %i: took place on the seat\tTime: %lo\n", pid, get_time());
            }

            release_semaphore(semaphore_id);
        }

        while (client_status == CLIENT_SITTING) {
            if (client_status == CLIENT_SITTING && barber_shop->current_client != pid) {
                client_status = CLIENT_SHAVED;
                printf("Client %i: shaved\t\t\tTime: %lo\n", pid, get_time());
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 3)
        ext("Not enough arguments\n");
    int clients_number = (int) strtol(argv[1], 0, 10);
    int cuts = (int) strtol(argv[2], 0, 10);

    init();

    for (int i = 0; i < clients_number; ++i) {
        if (!fork()) {
            run_client(cuts);
            exit(0);
        }
    }
    while (wait(0))
        if (errno != ECHILD)
            break;
}

