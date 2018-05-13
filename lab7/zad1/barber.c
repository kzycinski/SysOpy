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
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "common.h"

int mem_barbershop_id;
int sem_waiting_room_id;
int sem_chair_id;
int sem_shaving_id;
int sem_shaving_room_id;
struct barbershop *shared_barbershop;
int should_wake_up;


void init_barber(int waiting_room_size) {

    char *home_path = getenv("HOME");

    key_t waiting_room_key = ftok(home_path, SEM_WAITING_ROOM_ID);
    key_t chair_key = ftok(home_path, SEM_CHAIR_ID);
    key_t shaving_key = ftok(home_path, SEM_SHAVING_ID);
    key_t shaving_room_key = ftok(home_path, SEM_SHAVING_ROOM_ID);

    sem_waiting_room_id = semget(waiting_room_key, 1, IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR);
    sem_chair_id = semget(chair_key, 1, IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR);
    sem_shaving_id = semget(shaving_key, 1, IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR);
    sem_shaving_room_id = semget(shaving_room_key, 1, IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR);

    semctl(sem_waiting_room_id, 0, SETVAL, 0);
    semctl(sem_chair_id, 0, SETVAL, 0);
    semctl(sem_shaving_id, 0, SETVAL, 0);
    semctl(sem_shaving_room_id, 0, SETVAL, 0);

    key_t barbershop_key = ftok(home_path, MEM_BARBERSHOP_ID);
    mem_barbershop_id = shmget(barbershop_key, sizeof(struct barbershop), IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR);
    shared_barbershop = (struct barbershop*) shmat(mem_barbershop_id, 0, 0);

    if(shared_barbershop == (void*)-1) {
        perror("Barber - shmat\n");
        exit(1);
    }


    shared_barbershop -> empty_sits_number = waiting_room_size;
    init_queue(&(shared_barbershop -> clients_queue), waiting_room_size);
    shared_barbershop -> barber_pid = getpid();
    shared_barbershop -> is_sleeping = 0;
    shared_barbershop -> shaved_client_pid = -1;
}

void fall_asleep() {

    shared_barbershop -> is_sleeping = 1;

    sigset_t tmp_mask, old_mask;
    sigemptyset(&tmp_mask);
    sigaddset(&tmp_mask, SIG_WAKEUP);
    sigprocmask(SIG_BLOCK, &tmp_mask, &old_mask);

    print_msg("Falling asleep\n");
    should_wake_up = 0;
    increase_sem(sem_waiting_room_id);

    while(should_wake_up == 0)
        sigsuspend(&old_mask);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    shared_barbershop -> is_sleeping = 0;
}

void shave_client() {

    increase_sem(sem_shaving_id);
    increase_sem(sem_shaving_room_id);

    increase_sem(sem_chair_id);
    wait_sem(sem_chair_id);

    print_msg("");
    fprintf(stdout, "Started shaving %d\n", shared_barbershop -> shaved_client_pid);

    print_msg("");
    fprintf(stdout, "Finished shaving %d\n", shared_barbershop -> shaved_client_pid);

    decrase_sem(sem_shaving_id);
    decrase_sem(sem_shaving_room_id);
    decrase_sem(sem_chair_id);
}

int invite_client() {

    decrase_sem(sem_waiting_room_id);

    pid_t next_client_pid = dequeue(&(shared_barbershop -> clients_queue));

    if(next_client_pid == -1)
        return 0;

    shared_barbershop -> empty_sits_number++;
    print_msg("");
    fprintf(stdout, "Inviting %d from queue\n", next_client_pid);
    kill(next_client_pid, SIG_INVITE);
    increase_sem(sem_waiting_room_id);
    return 1;
}

static void at_exit() {

    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } none;

    if(mem_barbershop_id != -1) {

        if(shmctl(mem_barbershop_id, IPC_RMID, NULL) == -1)
            perror("Barber - shmctl\n");
    }

    if(sem_waiting_room_id != -1) {

        if(semctl(sem_waiting_room_id, 0, IPC_RMID, none) == -1)
            perror("Barber - semctl waiting room\n");
    }

    if(sem_chair_id != -1) {

        if(semctl(sem_chair_id, 0, IPC_RMID, none) == -1)
            perror("Barber - semctl chair\n");    }

    if(sem_shaving_id != -1) {

        if(semctl(sem_shaving_id, 0, IPC_RMID, none) == -1)
            perror("Barber - semctl shaving\n");    }

    if(sem_shaving_room_id != -1) {

        if(semctl(sem_shaving_room_id, 0, IPC_RMID, none) == -1)
            perror("Barber - semctl shaving room\n");    }
}

void handler_sigint(int signo) {

    fprintf(stderr, "Aborting...\n");
    exit(1);
}

void handler_sigterm(int signo) {

    fprintf(stderr, "Aborting...\n");
    exit(0);
}

void handler_sigwakeup(int signo) {

    should_wake_up = 1;
    print_msg("Waking up\n");
}

int main(int argc, char** argv) {

    mem_barbershop_id = -1;
    sem_waiting_room_id = -1;
    sem_chair_id = -1;
    sem_shaving_id = -1;
    sem_shaving_room_id = -1;
    should_wake_up = 0;

    if (atexit(at_exit) != 0) {

        perror("Barber - atexit err\n");
        exit(1);
    }

    struct sigaction act;
    act.sa_handler = handler_sigint;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if(sigaction(SIGINT, &act, NULL) == -1) {

        perror("Barber - sigint\n");
        exit(1);
    }


    act.sa_handler = handler_sigterm;

    if(sigaction(SIGTERM, &act, NULL) == -1) {

        perror("Barber - sigterm\n");
        exit(1);
    }


    act.sa_handler = handler_sigwakeup;

    if(sigaction(SIG_WAKEUP, &act, NULL) == -1) {

        perror("Barber - sigwakeup\n");
        exit(1);
    }

    if(argc != 2) {
        printf("Wrong arguments\n");
        exit(1);
    }

    int waiting_room_size = (int) strtol(argv[1], NULL, 10);

    init_barber(waiting_room_size);

    while(1) {

        fall_asleep();
        shave_client();

        while(invite_client() == 1)
            shave_client();
    }
}
