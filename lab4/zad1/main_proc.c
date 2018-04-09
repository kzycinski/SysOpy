#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int is_paused = 0;
pid_t child_pid;

void start_child();

void sigtstp_handler(int sig_no){
    if(!is_paused) {
        kill(child_pid, SIGKILL);
        printf("\nOczekuję na CTRL+Z - kontynuacja albo CTR+C - zakonczenie programu\n");
    } else {
        start_child();
    }
    is_paused = is_paused == 1 ? 0 : 1;
}

void sigint_handler(int sig_no){
    printf("\nOdebrano sygnał SIGINT\n");
    exit(0);
}

void start_child(){
    pid_t pid = fork();
    if (pid < 0){
        printf("Fork failed!\n");
        exit(1);
    } else if(pid == 0){
        while(1) {
            static char* parameters[2] = {"date", NULL};
            execvp(parameters[0], parameters);
            exit(0);
        }
    } else {
        child_pid = pid;
    }
}

int main() {

    struct sigaction act;
    act.sa_handler = sigtstp_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGTSTP, &act, NULL);
    signal(SIGINT, sigint_handler);

    while (1) {
        if (!is_paused) {
            start_child();
        }
        sleep(1);
    }
}