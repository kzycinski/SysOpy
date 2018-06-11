#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <signal.h>

#include <arpa/inet.h>
#include "common.h"

int scket;
char *name;

void ext(char *string){
    perror(string);
    exit(1);
}

void send_msg(uint8_t message_type){
    uint16_t message_size = (uint16_t) (strlen(name) + 1);
    if(write(scket, &message_type, 1) != 1)
        ext("Client - write mes type\n");
    if(write(scket, &message_size, 2) != 2)
        ext("Client - write mes size\n");
    if(write(scket, name, message_size) != message_size)
        ext("Client - write mes name\n");
}

void calculate(){
    operation_t operation;
    result_t result;


    if(read(scket, &operation, sizeof(operation_t)) != sizeof(operation_t))
        ext("Client - read request msg\n");


    result.operation = operation.operation;
    result.value = 0;
    result.wrong_flag = 0;
    double res = 0;
    switch (operation.operand)
    {
        case '+':
            res = operation.arg1 + operation.arg2;
            break;
        case '-':
            res = operation.arg1 - operation.arg2;
            break;
        case '/':
            if (operation.arg2) {
                res = (operation.arg1 / operation.arg2);
                break;
            }
            else {
                result.wrong_flag = 1;
                break;
            }
        case '*':
            res = operation.arg1 * operation.arg2;
            break;
        default:
            result.wrong_flag = 1;
            break;
    }
    result.value = res;


    send_msg(RESULT);
    if(write(scket, &result, sizeof(result_t)) != sizeof(result_t))
        ext("Client - write result\n");
}

void server_login(){
    send_msg(LOGIN);

    uint8_t message_type;
    if(read(scket, &message_type, 1) != 1)
        ext("Client - read response type");
    switch(message_type){
        case WRONG_NAME:
            ext("Client -name is taken\n");
        case WRONG_SIZE:
            ext("Client - max client reached\n");
        case SUCCESS:
            printf("Logged in\n");
            break;
        default:
            ext("Client - register default\n");
    }
}




void read_msg(){
    uint8_t msg_type;
    while(1){
        if(read(scket, &msg_type, 1) != 1)
            ext("Client - read mess type\n");

        switch(msg_type){
            case REQUEST:
                calculate();
                break;
            case PING:
                send_msg(PONG);
                break;
            default:
                printf("Client - unknown mess type\n");
                break;
        }
    }
}
void sigint_handler(int signo){
    ext("SIGINT\n");
}

void init(char *arg1, char *arg2, char *arg3) {

    signal(SIGINT, sigint_handler);
    name = arg1;

    switch((strcmp(arg2, "WEB") == 0 ? WEB : strcmp(arg2, "LOCAL") == 0 ? LOCAL : -1)){
        case WEB: {
            strtok(arg3, ":");
            char *arg4 = strtok(NULL, ":");
            if(arg4 == NULL)
                ext("Client - IP\n");

            uint32_t ip = inet_addr(arg3);
            if (ip == -1)
                ext("Client - IP address\n");

            uint16_t port_num = (uint16_t) strtol(arg4, NULL, 10);
            if (port_num < 1024 || port_num > 65535)
                ext("Client - port number\n");

            struct sockaddr_in web_address;
            web_address.sin_family = AF_INET;
            web_address.sin_addr.s_addr = ip;
            web_address.sin_port = htons(port_num);

            if ((scket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                ext("Client - web socket\n");

            if (connect(scket, (const struct sockaddr *) &web_address, sizeof(web_address)) == -1)
                ext("Client - connect to web socket\n");

            break;
        }
        case LOCAL: {
            char* unix_path = arg3;
            if (strlen(unix_path) < 1 || strlen(unix_path) > UNIX_PATH_MAX)
                ext("Client - unix path\n");

            struct sockaddr_un local_address;
            local_address.sun_family = AF_UNIX;
            snprintf(local_address.sun_path, UNIX_PATH_MAX, "%s", unix_path);
            int tmp1 = (scket = socket(AF_UNIX, SOCK_STREAM, 0));
            if (tmp1 < 0)
                ext("Client - local socket\n");
            int tmp2 = connect(scket, (const struct sockaddr *) &local_address, sizeof(local_address));

            if (tmp2 == -1)
                ext("Client - connect to local socket\n");

            break;
        }
        default:
            ext("Client - init default");
    }

}

void clr() {
    send_msg(LOGOUT);
    if (shutdown(scket, SHUT_RDWR) == -1)
        fprintf(stderr, "Client - shutdown\n");
    if (close(scket) == -1)
        fprintf(stderr, "Client - close\n");
}

void print_info(){
    printf("Run command:\n");
    printf("./client <name> <WEB/LOCAL> <port/unix_path>\n");
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Wrong arguments!\n");
        print_info();
        exit(1);
    }
    if (atexit(clr) == -1)
        ext("Client - atexit\n");


    init(argv[1], argv[2], argv[3]);

    server_login();

    read_msg();

    return 0;
}
