#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>

#define EQUAL 1
#define LESS 2
#define GREATER 3

#define PRINT_ALL 1
#define PRINT_SIMPLE 2

int P, K, N, L, search_mode, print_mode, nk;
FILE *source_file;
char **buffer = NULL;
int write_to = 0, read_from = 0, buffer_slots_taken = 0;

sem_t slots_sem;
sem_t free_sem;
sem_t taken_sem;

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
    int mode;
    if (buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';
    switch (search_mode) {
        case EQUAL:
            mode = (strlen(buff) == L);
            break;
        case GREATER:
            mode = (strlen(buff) > L);
            break;
        case LESS:
            mode = (strlen(buff) < L);
            break;
        default:
            err("Wrong search_mode\n");
    }
    if (mode)
        printf("%d -- %s\n", read_from, buff);
}

void *producer(void *arg) {
    size_t n = 0;
    while (1) {
        char *buff = NULL;

        sem_wait(&free_sem);
        sem_wait(&slots_sem);

        if (getline(&buff, &n, source_file) <= 0) {
            sem_post(&slots_sem);
            break;
        }

        if (print_mode == PRINT_ALL)
            printf("Producer writes to: %i\n", write_to);

        buffer[write_to] = buff;
        write_to = (write_to + 1) % N;
        buffer_slots_taken++;

        sem_post(&taken_sem);
        sem_post(&slots_sem);

        n = 0;
        buff = NULL;
    }
    pthread_exit((void *) 0);
}

void *consumer(void *arg) {
    char *buff;
    while (1) {
        sem_wait(&taken_sem);
        sem_wait(&slots_sem);

        buff = buffer[read_from];
        buffer[read_from] = NULL;

        if (print_mode == PRINT_ALL)
            printf("Consument reads from %i\n", read_from);
        consumer_print(buff, read_from);

        read_from = (read_from + 1) % N;
        buffer_slots_taken--;

        sem_post(&free_sem);
        sem_post(&slots_sem);

        free(buff);
    }
    pthread_exit((void *) 0);
}

void ext() {
    if (buffer)
        free(buffer);
    if (source_file)
        fclose(source_file);

    sem_destroy(&slots_sem);
    sem_destroy(&free_sem);
    sem_destroy(&taken_sem);
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

    sem_init(&slots_sem, 0, 1);
    sem_init(&taken_sem, 0, 0);
    sem_init(&free_sem, 0, N);

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


    if (nk)
        sleep(nk);
    while (1)
    {
        sem_wait(&slots_sem);
        if (buffer_slots_taken == 0)
            break;
        sem_post(&slots_sem);
    }
    exit(EXIT_SUCCESS);
}