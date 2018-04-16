#include <stdlib.h>
#include <stdio.h>
#include <zconf.h>
#include <time.h>
#include <memory.h>


#define BUFFOR_SIZE 100
int main(int argc, char **argv) {
    if (argc != 3){
        printf("Wrong arguments!\n");
        exit(1);
    }

    char *p = argv[1];
    int N = (int)strtol(argv[2], NULL, 10);


    char *buffor = (char*) malloc(BUFFOR_SIZE * sizeof(char));
    char *pid = (char*) malloc(10 * sizeof(char));
    sprintf(pid, "%d", getpid());
    while (N > 0) {
        srand(time(NULL));
        FILE *pipe = fopen(p, "w");

        sleep(rand()%5 + 1);

        FILE *date = popen("date", "r");

        fgets(buffor, BUFFOR_SIZE, date);

        pclose(date);

        printf("Pid: %s -- %s",pid, buffor);
        fwrite("pid - ", 1, 6, pipe);
        fwrite(pid, 1, strlen(pid), pipe);
        fwrite(", ", 1, 2, pipe);
        fwrite(buffor, 1, BUFFOR_SIZE, pipe);

        fclose(pipe);
        N--;
    }

    free(pid);
    free(buffor);
}