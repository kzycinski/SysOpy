#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include "common.h"

int web_socket;
int local_socket;
int epoll;
char *unix_path;

pthread_t ping_thread;
pthread_t command_thread;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
Client clients[CLIENT_MAX];
int client_counter = 0;
int operation_counter = 0;


void ext(char *string){
    perror(string);
    exit(EXIT_FAILURE);
}
int check_name(const char *name){
    for (int i = 0; i < client_counter; ++i){
        if (strcmp(clients[i].name,name) == 0) {
            return i;
        }
    }

    return -1;
}


void remove_client(int i) {
    free(clients[i].sockaddr);
    free(clients[i].name);


    client_counter--;
    for (int j = i; j < client_counter; ++j)
        clients[j] = clients[j + 1];

}

void logout_client(char *client_name){
    pthread_mutex_lock(&clients_mutex);
    int i = check_name(client_name);
    if(i >= 0){
        remove_client(i);
        printf("Client \"%s\" logout\n", client_name);
    }
    pthread_mutex_unlock(&clients_mutex);
}


void login_client(int socket, message_t msg, struct sockaddr *sockaddr, socklen_t socklen){
    uint8_t msg_type;
    pthread_mutex_lock(&clients_mutex);
    if(client_counter == CLIENT_MAX){
        msg_type = WRONG_SIZE;
        if (sendto(socket, &msg_type, 1, 0, sockaddr, socklen) != 1)
            ext("Server - write wrong_size\n");
        free(sockaddr);
    } else {
        int exists = check_name(msg.name);
        if(exists != -1){
            msg_type = WRONG_NAME;
            if (sendto(socket, &msg_type, 1, 0, sockaddr, socklen) != 1)
                ext("Server - write wrong_name\n");
            free(sockaddr);
        } else {
            clients[client_counter].sockaddr = sockaddr;
            clients[client_counter].connect_type = msg.connect_type;
            clients[client_counter].socklen = socklen;
            clients[client_counter].name = malloc(strlen(msg.name) + 1);
            clients[client_counter].is_inactive = 0;
            strcpy(clients[client_counter++].name, msg.name);
            msg_type = SUCCESS;
            if (sendto(socket, &msg_type, 1, 0, sockaddr, socklen) != 1)
                ext("Server - write success\n");
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}


void sigint_handler(int signo) {
    ext("Aborting\n");
}

void *command_handler(void *arg) {
    srand((unsigned int) time(NULL));
    operation_t msg;
    uint8_t message_type = REQUEST;
    int error = 0;
    char buffer[256];
    while (1) {
        printf("Enter order: \n");
        fgets(buffer, 256, stdin);
        if (sscanf(buffer, "%lf %c %lf", &msg.arg1, &msg.operand, &msg.arg2) != 3) {
            printf("Wrong format\n");
            printf("Correct format: arg1 <operator> arg2\n");
            continue;
        }
        if (msg.operand != '+' && msg.operand != '-' && msg.operand != '*' && msg.operand != '/') {
            printf("Wrong operator\n");
            printf("Available operators: + - * /\n");
            continue;
        }
        msg.operation = ++operation_counter;
        pthread_mutex_lock(&clients_mutex);
        if(client_counter == 0){
            printf("There are no clients connected\n");
            continue;
        }
        error = 0;
        int i = rand() % client_counter;

        int socket = clients[i].connect_type == WEB ? web_socket : local_socket;

        if (sendto(socket, &message_type, 1, 0, clients[i].sockaddr, clients[i].socklen) != 1) error = 1;
        if (sendto(socket, &msg, sizeof(operation_t), 0, clients[i].sockaddr, clients[i].socklen) !=
            sizeof(operation_t)) error = 1;
        if (error == 0)
            printf("Command %d: %lf %c %lf Has been sent to client \"%s\"\n",
                   msg.operation, msg.arg1, msg.operand, msg.arg2, clients[i].name);
        else
            printf("Cannot send order to \"%s\"\n", clients[i].name);
        pthread_mutex_unlock(&clients_mutex);

    }
}


void *ping_clients(void *arg) {
    uint8_t msg_type = PING;
    while (1) {
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < client_counter; ++i) {
            if (clients[i].is_inactive != 0) {
                printf("Client \"%s\" is inactive, removing from list.\n", clients[i].name);
                remove_client(i--);
            } else {
                int socket = clients[i].connect_type == WEB ? web_socket : local_socket;
                if (sendto(socket, &msg_type, 1, 0, clients[i].sockaddr, clients[i].socklen) != 1)
                    ext("Server - cannot ping clients\n");
                clients[i].is_inactive++;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        sleep(PING_TIME);
    }
}


void msg(int socket) {
    struct sockaddr* sockaddr = malloc(sizeof(struct sockaddr));
    socklen_t socklen = sizeof(struct sockaddr);
    message_t msg;

    if(recvfrom(socket, &msg, sizeof(message_t), 0, sockaddr, &socklen) != sizeof(message_t))
        ext("Server - read msg type\n");


    switch(msg.msg_type){
        case LOGIN:{
            login_client(socket, msg, sockaddr, socklen);
            break;
        }
        case LOGOUT:{
            logout_client(msg.name);
            break;
        }
        case RESULT:{
            if(msg.wrong_flag == 1){
                printf("Error - divide by 0\n");
                break;
            }
            printf("%d:Result: %lf\t\tClient - \"%s\"\n", msg.op, msg.val, msg.name);
            break;
        }
        case PONG:{
            pthread_mutex_lock(&clients_mutex);
            int i = check_name(msg.name);
            if (i >= 0)
                clients[i].is_inactive--;
            pthread_mutex_unlock(&clients_mutex);
            break;
        }
        default:
            printf("Unknown message type\n");
            break;
    }
}


void server_init(char *arg1, char *arg2) {
    signal(SIGINT, sigint_handler);

    uint16_t port_num = (uint16_t) strtol(arg1, NULL, 10);
    if (port_num < 1024 || port_num > 65535)
        ext("Wrong port number!\n");

    unix_path = arg2;
    if (strlen(unix_path) < 1 || strlen(unix_path) > UNIX_PATH_MAX)
        ext("Wrong unix path!\n");

    struct sockaddr_in web_address;
    web_address.sin_family = AF_INET;
    web_address.sin_addr.s_addr = htonl(INADDR_ANY);
    web_address.sin_port = htons(port_num);

    if ((web_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        ext("Server - create web_socket\n");

    if (bind(web_socket, (const struct sockaddr *) &web_address, sizeof(web_address)))
        ext("Server - bind web socket\n");

    struct sockaddr_un local_address;
    local_address.sun_family = AF_UNIX;

    snprintf(local_address.sun_path, UNIX_PATH_MAX, "%s", unix_path);

    if ((local_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
        ext("Server - create local socket\n");

    if (bind(local_socket, (const struct sockaddr *) &local_address, sizeof(local_address)))
        ext("Server - bind local socket\n");

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;

    if ((epoll = epoll_create1(0)) == -1)
        ext("Server - create epoll\n");

    event.data.fd = web_socket;
    if(epoll_ctl(epoll, EPOLL_CTL_ADD, web_socket, &event) == -1)
        ext("Server - add web socket to epoll\n");

    event.data.fd = local_socket;
    if(epoll_ctl(epoll, EPOLL_CTL_ADD, local_socket, &event) == -1)
        ext("Server - add local socket to epoll\n");

    if (pthread_create(&ping_thread, NULL, ping_clients, NULL) != 0)
        ext("Server - pinger thread\n");

    if (pthread_create(&command_thread, NULL, command_handler, NULL) != 0)
        ext("Server - command thread\n");
}

void clr() {
    pthread_cancel(ping_thread);
    pthread_cancel(command_thread);
    if (close(web_socket) == -1)
        fprintf(stderr, "Server - closing web socket\n");
    if (close(local_socket) == -1)
        fprintf(stderr, "Server - closing local socket\n");
    if (unlink(unix_path) == -1)
        fprintf(stderr, "Server - unlinking unix path\n");
    if (close(epoll) == -1)
        fprintf(stderr, "Server - closing epoll\n");
}
void print_usage() {
    printf("Run command:\n");
    printf("./server <port> <unix_path>\n");
}


int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage();
        ext("Invalid arguments!\n");
    }
    if (atexit(clr) == -1)
        ext("Server - atexit\n");

    server_init(argv[1], argv[2]);

    struct epoll_event event;
    while (1) {
        if (epoll_wait(epoll, &event, 1, -1) == -1)
            ext("Server - epoll wait\n");
        msg(event.data.fd);
    }
}