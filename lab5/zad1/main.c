#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>

void print_info() {
    printf("Program gets only one argument - filename with commands.\n");
}

char **parse(char *command){
    char **args = NULL;
    int arguments = 0;
    char a[3] = {' ','\n','\t'};
    char *tmp = strtok(command, " ");
    while (tmp) {
        printf("%s\n",tmp);
        arguments++;
        args = realloc(args, sizeof(char*) * arguments);
        args[arguments - 1] = tmp;
        tmp = strtok(NULL, a);
    }
    args = realloc(args, sizeof(char*) * (arguments+1));
    args[arguments]=0;
    return args;
}

int main(int argc, char const *argv[]) {

    if (argc < 2 || argc > 2) {
        printf("Enter valid arguments!\n");
        print_info();
        exit(1);
    }

    const int line_length = 1000;
    const int arg_length = 100;
    const int max_args = 10;
    int pipes[2][2];

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
        printf("Cannot open the file! \n");
        exit(1);
    }

    char *str = malloc(line_length * sizeof(char)); //err
    size_t length;
    int i = 0;

    while (getline(&str, &length, file) != -1) {

        char *commands[max_args];
        str[strlen(str) - 1] = '\0';
        char *command = strtok(str, "|");

        while (command) {
            command[arg_length - 1] = '\0';
            commands[i] = command;
            command = strtok(NULL, "|");
            i++;
        }

        int counter = i;
        printf("%d\n",counter);

        for (i = 0; i < counter; i++) {

            if (i > 1) {
                close(pipes[i % 2][0]);
                close(pipes[i % 2][1]);
            }

            if(pipe(pipes[i % 2]) == -1) {
                printf("Pipe error\n");
                exit(EXIT_FAILURE);
            }
            pid_t pid = fork();
            if (pid == 0) {
                char **args = parse(commands[i]);

                if ( i  <  counter - 1) {
                    close(pipes[i % 2][0]);
                    if (dup2(pipes[i % 2][1], 1) < 0) {
                        exit(EXIT_FAILURE);
                    };
                }
                if (i > 0) {
                    close(pipes[(i + 1) % 2][1]);
                    if (dup2(pipes[(i + 1) % 2][0], 0) < 0) {
                        close(EXIT_FAILURE);
                    }
                }
                execvp(args[0], args);

                exit(EXIT_SUCCESS);
            }
        }
        close(pipes[i % 2][0]);
        close(pipes[i % 2][1]);
        while (wait(0)) {
            if (errno != ECHILD) continue;
            printf("\n");
            break;
        }
        exit(0);
    }
}
