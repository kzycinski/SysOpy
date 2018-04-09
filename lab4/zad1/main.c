#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int is_paused = 0;

void sigtstp_handler(int sig_no){
    if(!is_paused){
        printf("\nOczekuję na CTRL+Z - kontynuacja albo CTR+C - zakonczenie programu\n");
    }
    is_paused = is_paused == 1 ? 0 : 1;
}

void sigint_handler(int sig_no){
    printf("\nOdebrano sygnał SIGINT\n");
    exit(0);
}

int main() {

    struct sigaction act;
    act.sa_handler = sigtstp_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGTSTP, &act, NULL);
    signal(SIGINT, sigint_handler);

    time_t time_buff;
    struct tm *time_info;

    while (1) {
        if (!is_paused) {
            time_buff = time(NULL);
            time_info = localtime(&time_buff);
            printf("\n%s", asctime(time_info));
        }
        sleep(1);
    }
}