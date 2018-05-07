#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <zconf.h>
#include <semaphore.h>
#include <errno.h>

#include "common.h"


int shared_memory_fd;
sem_t *semaphore;

void clean_up() {
    if (semaphore != NULL) {
        sem_close(semaphore);
        sem_unlink(PROJECT_PATH);
    }

    if (shared_memory_fd != 0) {
        close(shared_memory_fd);
        shm_unlink(PROJECT_PATH);
    }
}

void handle_signal(int _) {
    clean_up();
    printf("\nAborting...\n");
    exit(0);
}

void invite_client() {
    pid_t client_pid = barber_shop->queue[0];
    barber_shop->current_client = client_pid;
    printf("Invited client %i\tTime: %lo\n", client_pid, get_time());

    for (int i = 0; i < barber_shop->clients - 1; ++i) {
        barber_shop->queue[i] = barber_shop->queue[i + 1];
    }
    barber_shop->queue[barber_shop->clients - 1] = 0;
    barber_shop->clients -= 1;
}

void serve_client() {
    printf("Shaving client %i\tTime: %lo\n", barber_shop->current_client, get_time());

    printf("Finished client %i\tTime: %lo\n", barber_shop->current_client, get_time());

    barber_shop->current_client = 0;
}


void init(int argc, char **argv) {

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    atexit(clean_up);

    if (argc < 2)
        ext("Not enough arguments \n");


    int waiting_room_size = (int) strtol(argv[1], 0, 10);
    if (waiting_room_size > MAX_QUEUE_SIZE) {
        char buffer[50];
        sprintf(buffer, "Waiting room size has to be less than %i \n", MAX_QUEUE_SIZE);
        ext(buffer);
    }


    shared_memory_fd = shm_open(PROJECT_PATH, O_CREAT | O_EXCL | O_RDWR, 0774);

    if (shared_memory_fd == -1)
    {
        printf("%d\n", errno);
        ext("Barber - shm_open\n");
    }

    if (ftruncate(shared_memory_fd, sizeof(struct Barbershop)) < 0)
        ext("Ftruncate");

    barber_shop = mmap(NULL, sizeof(struct Barbershop), \
                                 PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, shared_memory_fd, 0);

    if (barber_shop == MAP_FAILED)
        ext("Barber - mmap\n");

    sem_t *xd = sem_open(PROJECT_PATH, O_CREAT | O_EXCL | O_RDWR, 0774, 0);

    if (xd == SEM_FAILED) {
        printf("%d\n", errno);
        ext("Barber - sem_open\n");
    }

    semaphore = xd;

    barber_shop->barber_status = BARBER_SLEEPING;
    barber_shop->queue_size = waiting_room_size;

    barber_shop->clients = 0;
    barber_shop->current_client = 0;

    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
        barber_shop->queue[i] = 0;

}

int main(int argc, char **argv) {

    init(argc, argv);

    while (1) {
        sem_post(semaphore);
        switch (barber_shop->barber_status) {
            case BARBER_IDLE:
                if (is_full()) {
                    printf("Falls asleep\t\tTime: %lo\n", get_time());
                    barber_shop->barber_status = BARBER_SLEEPING;
                } else {
                    invite_client();
                    barber_shop->barber_status = BARBER_BUSY;
                }
                break;
            case BARBER_AWOKEN:
                printf("Woke up\t\t\tTime: %lo\n", get_time());
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
        sem_wait(semaphore);
    }
}