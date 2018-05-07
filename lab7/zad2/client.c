#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <zconf.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <semaphore.h>

#include "common.h"


int client_status;
int shared_memory_fd;
sem_t *semaphore;


void init() {
    shared_memory_fd = shm_open(PROJECT_PATH, O_RDWR, 0774);
    if (shared_memory_fd == -1)
        ext("Client - shm_open\n");

    barber_shop = mmap(0, sizeof(struct Barbershop), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_fd, 0);

    if (barber_shop == (void *) -1)
        ext("Client - mmap\n");

    semaphore = sem_open(PROJECT_PATH, 0);
    if (semaphore == SEM_FAILED)
        ext("Client - sem_open\n");
}

void run_client(int cuts) {
    pid_t pid = getpid();
    for (int j = 0; j < cuts; ++j) {
        client_status = CLIENT_NEW;

        sem_post(semaphore);

        if (barber_shop->barber_status == BARBER_SLEEPING) {
            if (barber_shop->current_client != 0)
                ext("Barber cannot shave while sleeping!\n");

            printf("Client %i: Waking up barber\t\tTime: %lo\n", pid, get_time());
            barber_shop->barber_status = BARBER_AWOKEN;

            barber_shop->current_client = pid;

        } else if (!is_full()) {
            enter_queue(pid);
            printf("Client %i: Entering queue\t\tTime: %lo\n", pid, get_time());
        } else {
            printf("Client %i: Queue full, client leaves\tTime: %lo\n", pid, get_time());
            sem_wait(semaphore);
            exit(0);
        }
        sem_wait(semaphore);

        while (client_status ==  CLIENT_NEW) {
            sem_post(semaphore);
            if (barber_shop->current_client == pid) {
                client_status = CLIENT_SITTING;
                printf("Client %i: Sitting on chair\t\tTime: %lo\n", pid, get_time());
            }

            sem_wait(semaphore);
        }

        while (client_status == CLIENT_SITTING) {
            if (client_status == CLIENT_SITTING && barber_shop->current_client != pid) {
                client_status = CLIENT_SHAVED;
                printf("Client %i: Shaved\t\t\tTime: %lo\n", pid, get_time());
            }
        }
    }
}


int main(int argc, char **argv) {
    if (argc < 3)
        ext("Not enough arguments \n");

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