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

int invited;

void init_client() {

    char *home_path = getenv("HOME");

    key_t waiting_room_key = ftok(home_path, SEM_WAITING_ROOM_ID);
    key_t chair_key = ftok(home_path, SEM_CHAIR_ID);
    key_t shaving_key = ftok(home_path, SEM_SHAVING_ID);
    key_t shaving_room = ftok(home_path, SEM_SHAVING_ROOM_ID);

    sem_waiting_room_id = semget(waiting_room_key, 0, 0);
    sem_chair_id = semget(chair_key, 0, 0);
    sem_shaving_id = semget(shaving_key, 0, 0);
    sem_shaving_room_id = semget(shaving_room, 0, 0);

    key_t barbershop_key = ftok(home_path, MEM_BARBERSHOP_ID);
    mem_barbershop_id = shmget(barbershop_key, 0, 0);
    shared_barbershop = (struct barbershop*) shmat(mem_barbershop_id, 0, 0);

    if(shared_barbershop == (void*)-1) {
        perror("Client - shmat err");
        exit(1);
    }
}

void wake_barber() {

    print_msg("Waking up the barber\n");
    kill(shared_barbershop -> barber_pid, SIG_WAKEUP);
}

void wait_in_queue() {

    enqueue(&(shared_barbershop -> clients_queue), getpid());
    shared_barbershop -> empty_sits_number--;

    sigset_t tmp_mask, old_mask;
    sigemptyset(&tmp_mask);
    sigaddset(&tmp_mask, SIG_INVITE);
    sigprocmask(SIG_BLOCK, &tmp_mask, &old_mask);

    print_msg("Sitting in queue\n");

    invited = 0;
    increase_sem(sem_waiting_room_id);

    while(invited == 0)
        sigsuspend(&old_mask);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}


void sit_on_chair() {

    decrase_sem(sem_shaving_room_id);
    print_msg("Taking a seat on barber chair\n");
    shared_barbershop -> shaved_client_pid = getpid();
    decrase_sem(sem_chair_id);
}

void shave() {

    wait_sem(sem_shaving_id);
    increase_sem(sem_chair_id);
    print_msg("Shaved, leaving\n");
    increase_sem(sem_shaving_room_id);
}


int enter_barbershop() {

    decrase_sem(sem_waiting_room_id);

    if(shared_barbershop -> is_sleeping == 1) {
        wake_barber();
        sit_on_chair();
        increase_sem(sem_waiting_room_id);
        shave();
    }

    else {

        if(shared_barbershop -> empty_sits_number > 0) {
            wait_in_queue();
            sit_on_chair();
            shave();
        }

        else {
            print_msg("Queue full, leaving\n");
            increase_sem(sem_waiting_room_id);
            return 0;
        }
    }

    return 1;
}

void run_clients(int shavings_number) {

    int shavings_received = 0;

    while(shavings_received < shavings_number)
        shavings_received += enter_barbershop();
}

void handler_siginvite(int signo) {

    invited = 1;
    print_msg("Entering shaving room\n");
}


int main(int argc, char** argv) {

    struct sigaction act;
    act.sa_handler = handler_siginvite;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if(sigaction(SIG_INVITE, &act, NULL) == -1) {

        perror("Client - siginvite");
        exit(1);
    }

    if(argc != 3) {
        printf("Wrong arguments!\n");
        exit(1);
    }

    int clients_number = (int)strtol(argv[1], NULL, 10);
    int shavings_number = (int)strtol(argv[2], NULL, 10);

    init_client();

    for(int i = 0; i < clients_number; i++) {
        pid_t id = fork();

        if(id == 0) {
            run_clients(shavings_number);
            return 0;
        }
    }

    for(int i = 0; i < clients_number; i++)
        wait(NULL);
}
