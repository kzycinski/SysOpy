#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>

#define EQUAL 1
#define LESS 2
#define GREATER 3

#define PRINT_ALL 1
#define PRINT_SIMPLE 2

int P, K, N, L, search_mode, print_mode, nk;
FILE *source_file;
char **buffer = NULL;
int write_to = 0, read_from = 0, buffer_slots_taken = 0, client_exit_count = 0, can_exit = 0;

pthread_mutex_t count_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty_buffer_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t full_buffer_cond = PTHREAD_COND_INITIALIZER;

void print_info(){
    printf("Config file format:\n");
    printf("<producers> <customers> <buffer_size> <source_file> <L> <search_mode> <print_mode> <nk>\n");
    printf("Search mode format - GT EQ LT\n");
    printf("Print mode format - ALL SIMPLE\n");
}
void sigexit_handler(int sig) {
    exit(EXIT_FAILURE);
}

void alarm_handler(int sig) {
    fprintf(stderr, "%d seconds passed, exiting...\n", nk);
    exit(EXIT_SUCCESS);
}

void err(const char *msg) {
    printf("%s\n", msg);
    print_info();
    exit(EXIT_FAILURE);
}

void consumer_print(char *buff, int read_from) {
    int flag;
    if (buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';
    switch (search_mode) {
        case EQUAL:
            flag = (strlen(buff) == L);
            break;
        case GREATER:
            flag = (strlen(buff) > L);
            break;
        case LESS:
            flag = (strlen(buff) < L);
            break;
        default:
            err("Wrong search_mode\n");
    }
    if (flag)
        printf("%i: %s\n", read_from, buff);
}

void *producer(void *arg) {
    size_t n = 0;
    while (1) {
        char *buff = NULL;

        pthread_mutex_lock(&count_buffer_mutex);
        while (buffer_slots_taken >= N)
            pthread_cond_wait(&full_buffer_cond, &count_buffer_mutex);

        if (getline(&buff, &n, source_file) <= 0) {
            pthread_mutex_unlock(&count_buffer_mutex);
            break;
        }

        if (print_mode == PRINT_ALL)
            printf("Producer puts a line into %i \t%i / %i\n", write_to, buffer_slots_taken + 1, N);

        buffer[write_to] = buff;
        write_to = (write_to + 1) % N;
        buffer_slots_taken++;

        pthread_cond_signal(&empty_buffer_cond);
        pthread_mutex_unlock(&count_buffer_mutex);

        n = 0;
        buff = NULL;
    }
    pthread_exit((void *) 0);
}

void *consumer(void *arg) {
    char *buff;
    while (1) {
        pthread_mutex_lock(&count_buffer_mutex);
        while (buffer_slots_taken <= 0) {
            if (can_exit) {
                client_exit_count++;
                pthread_exit((void *) 0);
            }
            pthread_cond_wait(&empty_buffer_cond, &count_buffer_mutex);
        }

        buff = buffer[read_from];
        buffer[read_from] = NULL;

        if (print_mode == PRINT_ALL)
            printf("Consument reads a line from %i \t%i / %i\n", read_from, buffer_slots_taken - 1, N);
        consumer_print(buff, read_from);

        read_from = (read_from + 1) % N;
        buffer_slots_taken--;

        pthread_cond_signal(&full_buffer_cond);
        pthread_mutex_unlock(&count_buffer_mutex);

        free(buff);
    }
    pthread_exit((void *) 0);
}

void ext() {
    if (buffer)
        free(buffer);
    if (source_file)
        fclose(source_file);

    pthread_mutex_destroy(&count_buffer_mutex);
    pthread_cond_destroy(&empty_buffer_cond);
    pthread_cond_destroy(&full_buffer_cond);
}

void load_config(char *const filepath) {
    FILE *file;
    if ((file = fopen(filepath, "r")) < 0)
        err("Cannot open command file\n");

    char buff[1024];
    fread(buff, 1024, 1, file);

    char *tmp = strtok(buff, " ");
    P = (int) strtol(tmp, NULL, 10);

    tmp = strtok(NULL, " ");
    K = (int) strtol(tmp, NULL, 10);

    tmp = strtok(NULL, " ");
    N = (int) strtol(tmp, NULL, 10);

    tmp = strtok(NULL, " ");
    if ((source_file = fopen(tmp, "r")) == NULL) err("Cannot open source file\n");

    tmp = strtok(NULL, " ");
    L = (int) strtol(tmp, NULL, 10);

    tmp = strtok(NULL, " ");
    search_mode = (strcmp(tmp, "LT") == 0) ? LESS : ((strcmp(tmp, "GT") == 0) ? GREATER : ((strcmp(tmp, "EQ") == 0))
                                                                                          ? EQUAL : 0);

    if (!search_mode)
        err("Wrong search mode!\n");

    tmp = strtok(NULL, " ");
    print_mode = (strcmp(tmp, "ALL") == 0) ? PRINT_ALL : ((strcmp(tmp, "SIMPLE") == 0) ? PRINT_SIMPLE : 0);

    if(!print_mode)
        err("Wrong print mode!\n");

    tmp = strtok(NULL, " ");
    nk = (int) strtol(tmp, NULL, 10);

    if (P <= 0 || K <= 0 || L < 0)
        err("Wrong command file\n");

    if (fclose(file) < 0)
        err("Cannot close command file\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) err("Too few arguments!");

    load_config(argv[1]);
    atexit(ext);
    signal(SIGINT, sigexit_handler);
    signal(SIGALRM, alarm_handler);


    pthread_t *producents = malloc(P * sizeof(pthread_t));
    pthread_t *consuments = malloc(K * sizeof(pthread_t));
    int i;

    buffer = malloc(N * sizeof(char *));

    for (i = 0; i < P; i++)
        if (pthread_create(&producents[i], NULL, &producer, NULL))
            err("Cannot create producents\n");
    for (i = 0; i < K; i++)
        if (pthread_create(&consuments[i], NULL, &consumer, NULL))
            err("Cannot create consumers");

    if (nk)
        alarm(nk);

    for (i = 0; i < P; i++)
        if (pthread_join(producents[i], NULL))
            err("Cannot join pthreads\n");
    can_exit = 1;

    while (1) {
        pthread_mutex_lock(&count_buffer_mutex);
        if (buffer_slots_taken == 0) {
            while (client_exit_count != K) {
                pthread_cond_signal(&empty_buffer_cond);
                pthread_mutex_unlock(&count_buffer_mutex);
            }
            if (nk)
                sleep(nk);
            break;
        }
        pthread_mutex_unlock(&count_buffer_mutex);
    }
    exit(EXIT_SUCCESS);
}