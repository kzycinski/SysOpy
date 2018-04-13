#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>


pid_t child_pid;
pid_t parent_pid;
int child_counter;
int parent_counter;
int type;
sem_t *semaphore;


void child_handler(int signal) {
    child_counter++;

    printf("%d: Child process: received %d\n", getpid(), signal);
    if (signal == SIGUSR1 || signal == SIGRTMIN){
        if (type == 3)
            kill(parent_pid, SIGRTMIN);
        else
            kill(parent_pid, SIGUSR1);
    } else if (signal == SIGUSR2 || signal == SIGRTMAX)

        exit(0);

    if (type == 1)
        sem_post(semaphore);
}

void parent_handler(int signal) {
    parent_counter++;
    printf("%d: Parent process: received %d\n", getpid(), signal);
    if (signal == SIGUSR1 || signal == SIGRTMIN) {
        if (type == 2)
            sem_post(semaphore);
    } else if (signal == SIGINT) {
        if(type == 3)
            kill(child_pid, SIGRTMAX);
        else
            kill(child_pid,SIGUSR2);

        munmap(semaphore, sizeof(sem_t));
        exit(0);
    }
}

void parent(int amount) {
    struct sigaction act;

    act.sa_handler = parent_handler;
    sigfillset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR1);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);

    if (type == 3)
        sigaction(SIGRTMIN, &act, NULL);
    else
        sigaction(SIGUSR1, &act, NULL);

    sem_wait(semaphore);
    sem_post(semaphore);

    for (int i = 0; i < amount; i++) {
        child_counter++;
        printf("%d: Parent process: send %d\n", getpid(), type == 3 ? SIGRTMIN : SIGUSR1);
        if (type != 3) {
            sem_wait(semaphore);
            kill(child_pid, SIGUSR1);
        }
        else
            kill(child_pid, SIGRTMIN);

        if (type == 2)
            sleep(1);
    }
    if (type == 3)
        kill(child_pid, SIGRTMAX);

    else kill(child_pid, SIGUSR2);

    alarm(1);

    while (parent_counter < amount)
        pause();

    sem_wait(semaphore);

    printf("Exit\n");
    munmap(semaphore, sizeof(sem_t));
}

void child() {
    struct sigaction act;

    act.sa_handler = child_handler;
    sigfillset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR1);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);
    if (type == 3) {
        sigaction(SIGRTMIN, &act, NULL);
        sigaction(SIGRTMAX, &act, NULL);
    } else {
        sigaction(SIGUSR1, &act, NULL);
        sigaction(SIGUSR2, &act, NULL);
    }

    sem_post(semaphore);
    while (1) {
        pause();
    }
}

int main(int argc, char const *argv[]) {
    if (argc < 3) {
        printf("Wrong number of arguments\n");
        exit(1);
    }
    int amount = (int) strtol(argv[1], NULL, 10);
    type = (int) strtol(argv[2], NULL, 10);

    if (amount <= 0 || type < 1 || type > 3) {
        printf("Wrong arguments\n");
        exit(1);
    }

    semaphore = (sem_t *) mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    sem_init(semaphore, 1, 1);


    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGUSR2);
    sigdelset(&mask, SIGRTMIN);
    sigdelset(&mask, SIGRTMAX);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    sem_wait(semaphore);
    parent_pid = getpid();
    child_counter = parent_counter = 0;

    child_pid = fork();

    if (child_pid == 0)
        child();
    else
        parent(amount);

    printf("Child recived: %d, parents recived: %d\n", child_counter,parent_counter);
    exit(0);
}