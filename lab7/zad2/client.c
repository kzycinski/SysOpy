#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "common.h"

int mem_barbershop;
sem_t *sem_waiting_room;
sem_t *sem_chair;
sem_t *sem_shaving;
sem_t *sem_shaving_room;

struct barbershop *barbershop;

int is_invited;

void init_client() {

    sem_waiting_room = sem_open(SEM_WTROOM_NAME, 0);
    sem_chair = sem_open(SEM_CHAIR_NAME, 0);
    sem_shaving = sem_open(SEM_SHAVING_NAME, 0);
    sem_shaving_room = sem_open(SEM_SHVROOM_NAME, 0);


    if (sem_waiting_room == SEM_FAILED || sem_chair == SEM_FAILED
        || sem_shaving == SEM_FAILED || sem_shaving_room == SEM_FAILED) {
        perror("Client - sem\n");
        exit(1);
    }

    mem_barbershop = shm_open(MEM_BRBSHOP_NAME, O_RDWR, 0);
    barbershop = (struct barbershop *) mmap(NULL, sizeof(struct barbershop), PROT_READ | PROT_WRITE, MAP_SHARED,
                                            mem_barbershop, 0);

    if (barbershop == (void *) -1) {
        perror("Client - mem\n");
        exit(1);
    }
}

void handler_sigwakeup() {

    print_msg("Waking up the barber\n");
    kill(barbershop->barber_pid, SIG_WAKEUP);
}

void wait_q() {

    enqueue(&(barbershop->clients_queue), getpid());
    barbershop->empty_sits_number--;

    sigset_t tmp_mask, old_mask;
    sigemptyset(&tmp_mask);
    sigaddset(&tmp_mask, SIG_INVITE);
    sigprocmask(SIG_BLOCK, &tmp_mask, &old_mask);

    print_msg("Sitting in queue\n");

    is_invited = 0;
    sem_post(sem_waiting_room);

    while (is_invited == 0)
        sigsuspend(&old_mask);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

void sit_on_chair() {

    sem_wait(sem_shaving_room);
    print_msg("Sitting on chair\n");
    barbershop->shaved_client_pid = getpid();
    sem_post(sem_chair);
}

void get_shaved() {

    sem_wait(sem_shaving);
    print_msg("Shaved - leaving\n");
}

int enter_barbershop() {

    sem_wait(sem_waiting_room);

    if (barbershop->is_sleeping == 1) {
        handler_sigwakeup();
        sit_on_chair();
        sem_post(sem_waiting_room);
        get_shaved();
    } else {

        if (barbershop->empty_sits_number > 0) {
            wait_q();
            sit_on_chair();
            get_shaved();
        } else {
            print_msg("Queue full, leaving\n");
            sem_post(sem_waiting_room);
            return 0;
        }
    }

    return 1;
}

void client_control(int shavings) {

    int times_shaved = 0;

    while (times_shaved < shavings)
        times_shaved += enter_barbershop();
}

void handler_siginvite(int signo) {
    is_invited = 1;
}

int main(int argc, char **argv) {

    struct sigaction act;
    act.sa_handler = handler_siginvite;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIG_INVITE, &act, NULL) == -1) {

        perror("Client - sigaction");
        exit(1);
    }

    if (argc != 3) {
        printf("Wrong arguments\n");
        exit(1);
    }

    int clients = (int) strtol(argv[1], NULL, 10);
    int shavings = (int) strtol(argv[1], NULL, 10);

    init_client();

    for (int i = 0; i < clients; i++) {
        pid_t id = fork();

        if (id == 0) {
            client_control(shavings);
            return 0;
        }
    }

    for (int i = 0; i < clients; i++)
        wait(NULL);
}
