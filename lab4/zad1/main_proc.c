#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int is_paused = 0;
static char* parameters[2] = {"./date.sh", 0};
int is_dead= 0;

void start_child();

void sigtstp_handler(int sig_no){
    if(!is_paused) {
        printf("\nOczekuję na CTRL+Z - kontynuacja albo CTR+C - zakonczenie programu\n");
    }
    is_paused = is_paused == 1 ? 0 : 1;
}

void sigint_handler(int sig_no){
    printf("\nOdebrano sygnał SIGINT\n");
    exit(0);
}

void start_child(){
    pid_t child_pid = fork();

    if (child_pid == 0){
        execvp(parameters[0], parameters);
        exit(0);
    }

    while(1){
        if(is_paused == 0) {
            if(is_dead){
                is_dead = 0;

                child_pid = fork();
                if (child_pid == 0){

                    execvp(parameters[0], parameters);
                    exit(0);
                }
            }
        } else {
            if (is_dead == 0) {
                kill(child_pid, SIGKILL);
                is_dead = 1;
            }
        }
    }
}


int main() {

    struct sigaction act;
    act.sa_handler = sigtstp_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGTSTP, &act, NULL);
    signal(SIGINT, sigint_handler);

    start_child();

}
