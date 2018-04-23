#define _GNU_SOURCE  500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <time.h>
#include <signal.h>

#include "numbers.h"

#define FAILURE_EXIT(code, format, ...) { fprintf(stderr, format, ##__VA_ARGS__); exit(code);}


int is_active = 1;
int clients [MAX_CLIENTS][2];
int client_count = 0;
mqd_t server_id = -1;


int find_id(pid_t pid){
    for(int i=0; i<client_count; i++){
        if(clients[i][0] == pid) return clients[i][1];
    }
    return -1;
}

int prepare_msg(struct msg *msg){
    int client_mqd = find_id(msg->client_pid);
    if(client_mqd == -1){
        printf("Wrong client code\n");
        return -1;
    }

    msg->msg_type = msg->client_pid;
    msg->client_pid = getpid();

    return client_mqd;
}

void remove_q(void){
    for(int i=0; i<client_count; i++){
        if(mq_close(clients[i][1]) == -1){
            printf("Error closing %d \n", i);
        }
        if(kill(clients[i][0], SIGINT) == -1){
            printf("Error killing %d \n", i);
        }
    }
    if(server_id > -1){
        if(mq_close(server_id) == -1){
            printf("Closing publicq failed\n");
        }

        mq_unlink(SERVER_NAME);

    }
    printf("\nClosing server...\n");
}

void intHandler(int signo){
    is_active = 0;
    exit(2);
}


void register_client(struct msg *msg){
    int client_pid = msg->client_pid;
    char client_path[15];
    sprintf(client_path, "/%d", client_pid);

    int client_mqd = mq_open(client_path, O_WRONLY);
    if(client_mqd == -1 ) FAILURE_EXIT(3, "Reading client_mqd failed!");

    msg->msg_type = INIT;
    msg->client_pid = getpid();

    if(client_count > MAX_CLIENTS - 1){
        printf("Max clients \n");
        sprintf(msg->msg_text, "%d", -1);
        if(mq_send(client_mqd, (char*) msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "Register failed");
        if(mq_close(client_mqd) == -1) FAILURE_EXIT(3, "Cant close client q");
    }else{
        clients[client_count][0] = client_pid;
        clients[client_count++][1] = client_mqd;
        sprintf(msg->msg_text, "%d", client_count-1);
        msg->msg_type = client_count-1;
        if(mq_send(client_mqd, (char*)msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "Cant login\n");
    }
}

void mirror(struct msg *msg){
    int client_mqd = prepare_msg(msg);
    if(client_mqd == -1) return;

    int msg_length = (int) strlen(msg->msg_text);
    if(msg->msg_text[msg_length-1] == '\n') msg_length--;

    for(int i=0; i < msg_length / 2; i++) {
        char buff = msg->msg_text[i];
        msg->msg_text[i] = msg->msg_text[msg_length - i - 1];
        msg->msg_text[msg_length - i - 1] = buff;
    }

    if(mq_send(client_mqd, (char*) msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "Mirror fail\n");
}

void calc(struct msg *msg){

    int client_mqd = prepare_msg(msg);
    if(client_mqd == -1) return;
    char opr;
    char *tmp = malloc(10 * sizeof(char));
    int num1, num2, result;
    int length = (int)strlen(msg->msg_text);
    int i = 5;
    while (msg->msg_text[i] >= '0' && msg->msg_text[i] <= '9' && i < length)
        i++;
    if (i == 0) {
        strcpy(msg->msg_text, "Bad calc syntax1\n");
        if(mq_send(client_mqd, (char*) msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "CALC fail\n");
        return;
    }

    sprintf(tmp, "%.*s",i-5, msg->msg_text+5);
    num1 = (int)strtol(tmp, NULL, 10);

    while (msg->msg_text[i] == ' ' && i < length)
        i++;
    if (i == length) {
        strcpy(msg->msg_text, "Bad calc syntax2\n");
        if(mq_send(client_mqd, (char*) msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "CALC fail\n");
        return;
    }

    opr = msg->msg_text[i++];

    while (msg->msg_text[i] == ' ' && i < length)
        i++;
    if (i == length || msg->msg_text[i] < '0' ||msg->msg_text[i] > '9') {
        printf("%d\n", i);
        printf("%s\n", msg->msg_text);
        strcpy(msg->msg_text, "Bad calc syntax3\n");
        if(mq_send(client_mqd, (char*) msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "CALC fail\n");
        return;
    }
    sprintf(tmp, "%.*s", length-i, msg->msg_text + i);
    num2 = (int)strtol(tmp, NULL, 10);
    switch (opr)
    {
        case '+':
            result = num1 + num2;
            sprintf(msg->msg_text, "%d", result);
            break;
        case '-':
            result = num1 - num2;
            sprintf(msg->msg_text, "%d", result);
            break;
        case '/':
            if (num2) {
                result = (num1 / num2);
                sprintf(msg->msg_text, "%d", result);
                break;
            }
            else {
                strcpy(msg->msg_text, "Can't divide by 0!\n");
                return;
            }
        case '*':
            result = num1 * num2;
            sprintf(msg->msg_text, "%d", result);
            break;
        default:
            strcpy(msg->msg_text, "Wrong sign!\n");
            break;
    }



    if(mq_send(client_mqd, (char*) msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "CALC fail\n");
}

void time_f(struct msg *msg){
    int client_mqd = prepare_msg(msg);
    if(client_mqd == -1) return;

    time_t seconds = time(NULL);
    char *tmp;
    tmp = ctime(&seconds);
    strcpy(msg->msg_text, tmp);
    if(mq_send(client_mqd, (char*) msg, MAX_MSG, 1) == -1) FAILURE_EXIT(3, "TIME fail");
}

void end_f(struct msg *msg){
    is_active = 0;
}
void function_handler(struct msg *msg){
    if(msg == NULL) return;
    switch(msg->msg_type){
        case INIT:
            register_client(msg);
            break;
        case MIRROR:
            mirror(msg);
            break;
        case CALC:
            calc(msg);
            break;
        case TIME:
            time_f(msg);
            break;
        case END:
            end_f(msg);
            break;
        default:
            break;
    }
}


int main(void){
    atexit(remove_q);
    signal(SIGINT, intHandler);
    struct mq_attr currentState;

    struct mq_attr posixAttr;
    posixAttr.mq_maxmsg = MAX_MQSIZE;
    posixAttr.mq_msgsize = MSG_SIZE;

    server_id = mq_open(SERVER_NAME, O_RDONLY | O_CREAT | O_EXCL, 0666, &posixAttr);
    if(server_id == -1) FAILURE_EXIT(3, "Server-cant create q\n");

    msg buff;
    while(1){
        if(is_active == 0){
            if(mq_getattr(server_id, &currentState) == -1) FAILURE_EXIT(3, "Cant read params\n");
            if(currentState.mq_curmsgs == 0) exit(0);
        }

        if(mq_receive(server_id,(char*) &buff, MSG_SIZE, NULL) == -1) FAILURE_EXIT(3, "Server- recive failed \n");
        function_handler(&buff);

    }
    return 0;
}