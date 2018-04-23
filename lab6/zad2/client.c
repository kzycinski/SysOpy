#define _GNU_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>

#include "numbers.h"

#define FAILURE_EXIT(code, format, ...) { fprintf(stderr, format, ##__VA_ARGS__); exit(code);}

int session = -2;
mqd_t server_id = -1;
mqd_t client_id = -1;
char my_id[20];

void remove_q(void) {
    if (client_id > -1) {

        mq_close(server_id);

        mq_close(client_id);

        mq_unlink(my_id);
    }
}

void intHandler(int signo) {
    exit(2);
}

void init() {
    msg msg;
    msg.msg_type = INIT;
    msg.client_pid = getpid();

    if (mq_send(server_id, (char *) &msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "Init send fail\n");
    if (mq_receive(client_id, (char *) &msg, MSG_SIZE, NULL) == -1) FAILURE_EXIT(3, "Init recive fail\n");
    sprintf(msg.msg_text, "%d", (int)msg.msg_type);
    if (sscanf(msg.msg_text, "%d", &session) < 1) FAILURE_EXIT(3, "cant scan login\n");
    if (session < 0) FAILURE_EXIT(3, "Max clients reached\n");

    printf("Assagning id: %d\n", session);
}

void mirror_func(struct msg *msg) {
    if (mq_send(server_id, (char *) msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "MIRROR fail(client)\n");
    if (mq_receive(client_id, (char *) msg, MSG_SIZE, NULL) == -1) FAILURE_EXIT(3, "Mirror recive fail(client)\n");
    printf("%s\n", msg->msg_text);
}

void parse_line(char *buf, msg *msg) {
    strcpy(msg->msg_text, buf);
    char *type = strtok(buf, " \n");
    if (strcmp(type, "MIRROR") == 0) {
        msg->msg_type = MIRROR;
        strcpy(msg->msg_text, strtok(NULL, " \n"));
    } else if (strcmp(type, "CALC") == 0) {
        msg->msg_type = CALC;


    } else if (strcmp(type, "TIME") == 0) {
        msg->msg_type = TIME;
        strcpy(msg->msg_text, "");
    } else if (strcmp(type, "END") == 0) {
        msg->msg_type = END;
        strcpy(msg->msg_text, "end");
    } else {
        msg->msg_type = UNKNOWN;
        strcpy(msg->msg_text, "");
    }
}

void calc_func(struct msg *msg) {
    if (mq_send(server_id, (char *) msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "CALC fail (client)\n");
    if (mq_receive(client_id, (char *) msg, MSG_SIZE, NULL) == -1) FAILURE_EXIT(3, "CALC recive fail (client)\n");
    printf("%s\n", msg->msg_text);
}

void time_func(struct msg *msg) {
    if (mq_send(server_id, (char *) msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "TIME fail (client)\n");
    if (mq_receive(client_id, (char *) msg, MSG_SIZE, NULL) == -1) FAILURE_EXIT(3, "TIME revice fail (client)\n");
    printf("%s\n", msg->msg_text);
}

void end_func(struct msg *msg) {

    if (mq_send(server_id, (char *) msg, MSG_SIZE, 1) == -1) FAILURE_EXIT(3, "END send failed(client)");
}


int main() {
    atexit(remove_q);
    signal(SIGINT, &intHandler);

    sprintf(my_id, "/%d", getpid());

    server_id = mq_open(SERVER_NAME, O_WRONLY);
    if (server_id == -1) FAILURE_EXIT(3, "Cant open publicq (client)\n");

    struct mq_attr posixAttr;
    posixAttr.mq_maxmsg = MAX_MQSIZE;
    posixAttr.mq_msgsize = MSG_SIZE;

    client_id = mq_open(my_id, O_RDONLY | O_CREAT | O_EXCL, 0666, &posixAttr);
    if (client_id == -1) FAILURE_EXIT(3, "Cant create priveteq(client)\n");

    init();

    char cmd[20];
    msg msg;
    while (1) {
        msg.client_pid = getpid();
        printf(">");
        if (fgets(cmd, 100, stdin) == NULL) {
            printf("STDIN error!\n");
            continue;
        }
        int n = (int) strlen(cmd);
        if (cmd[n - 1] == '\n') cmd[n - 1] = 0;

        parse_line(cmd, &msg);

        if (msg.msg_type == MIRROR) {
            mirror_func(&msg);
        } else if (msg.msg_type == CALC) {
            calc_func(&msg);
        } else if (msg.msg_type == TIME) {
            time_func(&msg);
        } else if (msg.msg_type == END) {
            end_func(&msg);
        } else printf("Wrong command\n");
    }
}