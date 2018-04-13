#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <zconf.h>
#include <signal.h>
#include <wait.h>

#define DIFF (SIGRTMAX - SIGRTMIN)

pid_t parent;
volatile sig_atomic_t  child_no = 0;
volatile sig_atomic_t  exit_flag = 0;
volatile sig_atomic_t  signals = 0;
int N;
int K;
pid_t *children;
pid_t *requests;


void sigrt_action(int signal, siginfo_t *siginfo, void *context) {
    if (getpid() != parent)
        return;
    printf("SIGMIN+%i from child: %i\n", signal - SIGRTMIN, siginfo->si_pid);
}

void sigint_handler(int signal, siginfo_t *siginfo, void *context) {
    if (getpid() != parent)
        return;
    printf("Received SIGINT.\n");
    for (int i = 0; i < child_no; ++i)
        kill(children[i], SIGKILL);
    exit_flag = 1;
}

void request_handler(int signal, siginfo_t *siginfo, void *context){
    if (getpid() != parent)
        return;
    printf("Request from %i.\n", siginfo->si_pid);
    signals++;
    if (signals < K) {
        requests[signals] = siginfo->si_pid;
    } else if (signals == K) {
        kill(siginfo->si_pid, SIGUSR1);
        for (int i = 0; i < K; ++i) kill(requests[i], SIGUSR1);
    } else if (signals > K) {
        kill(siginfo->si_pid, SIGUSR1);
    }
}
void sigusr1_handler(int signal, siginfo_t *siginfo, void *context) {
    printf("%i: Passed.\n", getpid());
    int sig_diff = rand() % DIFF;
    kill(getppid(), SIGRTMIN + sig_diff);
}

void sigchld_handler(int signal, siginfo_t *siginfo, void *context) {
    if (getpid() != parent)
        return;
    printf("Child %i stopped.\n", siginfo->si_pid);
    child_no--;
    for (int i = 0; i < N; ++i) {
        if (children[i] == siginfo->si_pid)
            children[i] = 0;
    }

    int exit_status;
    wait(&exit_status);
    exit_status = WIFEXITED(exit_status);

    printf("Child: %i, exited with status: %i \n", siginfo->si_pid, exit_status);
}


int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Enter proper arguments!");
        return 0;
    }

    N = (int) strtol(argv[1], NULL, 10);
    K = (int) strtol(argv[2], NULL, 10);

    children = malloc(N * sizeof(pid_t));
    requests = malloc(N * sizeof(pid_t));

    srand(time(NULL));

    pid_t pid;
    struct sigaction act;
    parent = getpid();

    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGUSR1);
    sigaddset(&signals, SIGCHLD);
    for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
        sigaddset(&signals, i);

    act.sa_sigaction = &sigrt_action;
    for (int i = 0; i < DIFF; ++i)
        sigaction(i, &act, 0);

    act.sa_sigaction = &(sigint_handler);
    sigaction((SIGINT), &act, 0);

    act.sa_sigaction = &(request_handler);
    sigaction((SIGUSR1), &act, 0);

    act.sa_sigaction = &(sigchld_handler);
    sigaction((SIGCHLD), &act, 0);

    for (int i = 0; i < N; i++) {
        pid = fork();
        child_no++;
        if (pid)
            children[i] = pid;
        else {
            int seconds = 1;//(rand() % 9 + 1);
            sleep(seconds);
            printf("%i: awoke, sending signal to parent.\n", getpid());
            kill(getppid(), SIGUSR1);

            act.sa_sigaction = &(sigusr1_handler);
            sigaction((SIGUSR1), &act, 0);

            pause();
            printf("%i: Dying.\n", getpid());
            return seconds;
        }
    }
    printf("Beggining\n");

    while (child_no) {
        if (exit_flag) //flag
            for (int i = 0; i < N; ++i) {
                if (children[i])
                    kill(children[i], SIGKILL);
            }
    }
    return 0;
}