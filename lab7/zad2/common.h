#define PROJECT_PATH "/barbersss8t2s3"

#define MAX_QUEUE_SIZE 100

void ext(char *msg){
    printf("%s\n", msg);
    exit(1);
}

#define BARBER_IDLE 0
#define BARBER_SLEEPING 1
#define BARBER_AWOKEN 2
#define BARBER_BUSY 3
#define CLIENT_NEW 4
#define CLIENT_SITTING 5
#define CLIENT_SHAVED 6

struct Barbershop {
    int barber_status;
    int clients;
    int queue_size;
    pid_t current_client;
    pid_t queue[MAX_QUEUE_SIZE];
} *barber_shop;

long get_time() {
    struct timespec buf;
    clock_gettime(CLOCK_MONOTONIC, &buf);
    return buf.tv_nsec / 1000;
}

int is_full() {
    if (barber_shop->clients < barber_shop->queue_size) return 0;
    return 1;
}

void enter_queue(pid_t pid) {
    barber_shop->queue[barber_shop->clients] = pid;
    barber_shop->clients += 1;
}


