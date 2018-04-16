#include <stdio.h>
#include <stdlib.h>
#include <zconf.h>
#include <signal.h>
#include <wait.h>


int childPid;
int masterPid;
int childPids[4096];


int main(int argc, char *argv[]) {

    char buffer[512];
    int slaveNumber;
    int N;
    struct sigaction sigAction;


    if (argc != 4){
        printf("Wrong args\n");
        exit(EXIT_FAILURE);
    }


    slaveNumber = (int) strtol(argv[2], NULL, 10);

    masterPid = fork();

    if (masterPid == 0){
        execlp("./master", "master", "myFifo", NULL);
    }

    for (int i = 0; i < slaveNumber; ++i) {
        childPids[i] = fork();
        if (childPids[i] == 0){
            execlp("./slave","slave", argv[1], argv[3], NULL);
        }
    }

    for (int i = 0; i < slaveNumber; ++i) {
        waitpid(childPids[i], NULL, WUNTRACED);
    }

    kill(masterPid, SIGINT);
    killpg(0, SIGINT);

    waitpid(masterPid, NULL, WUNTRACED);

    return 0;
}
