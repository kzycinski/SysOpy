#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <memory.h>
#include <stdlib.h>

void print_info() {
    printf("Program gets only one argument - filename with commands.\n");
}

int main(int argc, char const *argv[]) {

    if (argc < 2 || argc > 2) {
        printf("Enter valid arguments!\n");
        print_info();
        exit(1);
    }

    const int line_length = 100;
    const int max_args = 10;

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
        printf("Cannot open the file! \n");
        exit(1);
    }

    char *str = malloc(line_length * sizeof(char)); //err
    size_t length;


    while (getline(&str, &length, file) != -1) {
        char *args[max_args];
        str[strlen(str) - 1] = '\0';
        if (strlen(str) == 1) continue;
        int arguments = 0;
        char *tmp = strtok(str, " ");
        while (tmp) {
            arguments++;
            args[arguments - 1] = tmp;
            tmp = strtok(NULL, " ");
        }
        free(tmp);
        args[arguments] = 0;
        if (args[0] == NULL) 
            continue;

        pid_t pid = fork();

        if (pid == -1) {
            printf("Error while making fork()!\n");
            exit(1);
        }
        if (pid == 0) {
            execvp(args[0], args);
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (WEXITSTATUS(status))
                printf("Error while making command %s\n", args[0]);
        }
    }

    free(str);
    fclose(file);
    return 0;
}
