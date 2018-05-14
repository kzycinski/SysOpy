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
int should_wake_up;


void init_barber(int waiting_room_size) {

    sem_waiting_room = sem_open(SEM_WTROOM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    sem_chair = sem_open(SEM_CHAIR_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    sem_shaving = sem_open(SEM_SHAVING_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    sem_shaving_room = sem_open(SEM_SHVROOM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);

    mem_barbershop = shm_open(MEM_BRBSHOP_NAME, O_CREAT | O_EXCL | O_RDWR, S_IWUSR | S_IRUSR);
    ftruncate(mem_barbershop, sizeof(struct barbershop));


    barbershop = (struct barbershop *) mmap(NULL, sizeof(struct barbershop), PROT_READ | PROT_WRITE, MAP_SHARED,
                                            mem_barbershop, 0);

    if (barbershop == (void *) -1) {
        perror("Barber - memory\n");
        exit(1);
    }


    barbershop->empty_sits_number = waiting_room_size;
    init_queue(&(barbershop->clients_queue), waiting_room_size);
    barbershop->barber_pid = getpid();
    barbershop->is_sleeping = 0;
    barbershop->shaved_client_pid = -1;
}

void fall_asleep() {

    barbershop->is_sleeping = 1;

    sigset_t tmp_mask, old_mask;
    sigemptyset(&tmp_mask);
    sigaddset(&tmp_mask, SIG_WAKEUP);
    sigprocmask(SIG_BLOCK, &tmp_mask, &old_mask);

    print_msg("No clients, falling asleep\n");
    should_wake_up = 0;
    sem_post(sem_waiting_room);

    while (should_wake_up == 0)
        sigsuspend(&old_mask);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    barbershop->is_sleeping = 0;
}

void shave() {

    sem_post(sem_shaving_room);

    sem_wait(sem_chair);

    print_msg("");
    fprintf(stdout, "Started shaving: %d\n", barbershop->shaved_client_pid);

    print_msg("");
    fprintf(stdout, "Finished shaving: %d\n", barbershop->shaved_client_pid);

    sem_post(sem_shaving);

}

int invite() {

    sem_wait(sem_waiting_room);

    pid_t next_client_pid = dequeue(&(barbershop->clients_queue));

    if (next_client_pid == -1)
        return 0;

    barbershop->empty_sits_number++;
    print_msg("");
    fprintf(stdout, "Inviting %d \n", next_client_pid);
    kill(next_client_pid, SIG_INVITE);
    sem_post(sem_waiting_room);
    return 1;
}

static void exit_fun() {

    if (mem_barbershop != -1) {

        if (munmap(barbershop, sizeof(struct barbershop)))
            perror("Barber - munmap\n");

        if (shm_unlink(MEM_BRBSHOP_NAME) == -1)
            perror("Barber - unlink\n");
    }

    if (sem_waiting_room != (sem_t *) -1) {

        if (sem_close(sem_waiting_room) == -1)
            perror("Barber - semclose\n");

        if (sem_unlink(SEM_WTROOM_NAME) == -1)
            perror("Barber - semunlink\n");
    }

    if (sem_chair != (sem_t *) -1) {

        if (sem_close(sem_chair) == -1)
            perror("Barber - semclose\n");
        if (sem_unlink(SEM_CHAIR_NAME) == -1)
            perror("Barber - semunlink\n");
    }

    if (sem_shaving != (sem_t *) -1) {

        if (sem_close(sem_shaving) == -1)
            perror("Barber - semclose\n");
        if (sem_unlink(SEM_SHAVING_NAME) == -1)
            perror("Barber - semunlink\n");
    }

    if (sem_shaving_room != (sem_t *) -1) {

        if (sem_close(sem_shaving_room) == -1)
            perror("Barber - semclose\n");
        if (sem_unlink(SEM_SHVROOM_NAME) == -1)
            perror("Barber - semunlink\n");
    }

}

void handler_sigint(int signo) {

    fprintf(stderr, "Aborting...\n");
    exit(1);
}

void handler_sigterm(int signo) {

    fprintf(stderr, "Aborting...\n");
    exit(0);
}

void sig_wakeup(int signo) {

    should_wake_up = 1;
    print_msg("Waking up\n");
}


int main(int argc, char **argv) {

    mem_barbershop = -1;
    sem_waiting_room = (sem_t *) -1;
    sem_chair = (sem_t *) -1;
    sem_shaving = (sem_t *) -1;
    sem_shaving_room = (sem_t *) -1;
    should_wake_up = 0;

    if (atexit(exit_fun) != 0) {

        perror("Barber - atexit\n");
        exit(1);
    }

    struct sigaction act;
    act.sa_handler = handler_sigint;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGINT, &act, NULL) == -1) {

        perror("Barber - sigint\n");
        exit(1);
    }

    act.sa_handler = handler_sigterm;

    if (sigaction(SIGTERM, &act, NULL) == -1) {

        perror("Barber - sigterm\n");
        exit(1);
    }


    act.sa_handler = sig_wakeup;

    if (sigaction(SIG_WAKEUP, &act, NULL) == -1) {

        perror("Barber - sigwakeup\n");
        exit(1);
    }

    if (argc != 2) {
        printf("Wrong arguments\n");
        exit(1);
    }

    int queue_size = (int) strtol(argv[1], NULL, 10);

    init_barber(queue_size);

    while (1) {

        fall_asleep();
        shave();

        while (invite() == 1)
            shave();

    }
}
