#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/resource.h>

void print_info() {
    printf("Program gets 3 arguments\n");
    printf("1. Filename with commands to execute\n");
    printf("2. Processor time dedicated for each process in seconds\n");
    printf("3. Memory dedicated for each process in Megabytes\n");
}

float calculate_system_timediff(struct rusage *r_usage){

    float x = r_usage[1].ru_stime.tv_sec - r_usage[0].ru_stime.tv_sec;
    float y = r_usage[1].ru_stime.tv_usec - r_usage[0].ru_stime.tv_usec;
    return ((x * 1000000) + y) / 1000000;
}

//calculate user time in function call
float calculate_user_timediff(struct rusage *r_usage){

    float x = r_usage[1].ru_utime.tv_sec - r_usage[0].ru_utime.tv_sec;
    float y = r_usage[1].ru_utime.tv_usec - r_usage[0].ru_utime.tv_usec;
    return ((x * 1000000) + y) / 1000000;
}

void print_time(struct rusage *r_usage, char *command) {
    printf("Execution of %s\n", command);
    printf("User time: %f\n", calculate_user_timediff(r_usage));
    printf("System time: %f\n", calculate_system_timediff(r_usage));
}


void make_limits(int time, int size) {
    struct rlimit rlimit;

    rlimit.rlim_cur = rlimit.rlim_max = ((rlim_t)size * 1024 * 1024);
    setrlimit(RLIMIT_AS, &rlimit);

    rlimit.rlim_cur = rlimit.rlim_max = (rlim_t)time;
    setrlimit(RLIMIT_CPU, &rlimit);
}

int main(int argc, char const *argv[]) {

    if (argc < 4 || argc > 4) {
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

    int time = (int)strtol(argv[2], NULL, 10);
    int size = (int)strtol(argv[3], NULL, 10);

    make_limits(time, size);

    struct rusage *rusage = malloc (2 * sizeof(struct rusage));

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
        if(args[0] == NULL) continue;

        getrusage(RUSAGE_CHILDREN, &rusage[0]);

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
            if(WIFSIGNALED(status)) {
                printf("Execution of %s terminated because of limit exceed\n", args[0]);
                continue;
            }
            if (WEXITSTATUS(status)) {
                printf("Error while making command %s\n", args[0]);
                continue;
            }
        }
        getrusage(RUSAGE_CHILDREN, &rusage[1]);

        print_time(rusage,args[0]);
    }
    free(rusage);
    free(str);
    fclose(file);
    return 0;
}