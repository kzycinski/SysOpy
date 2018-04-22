#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <signal.h>
#include <sys/stat.h>

#include "msg_buf.h"



void sig_handler(int sig)
{
    if (sig == SIGINT)
    {
        printf("Interrupting server...\n");
        exit(EXIT_SUCCESS);
    }
    else if (sig == SIGSEGV)
        printf("Wtf");
}

void set_sigint()
{
    struct sigaction sig;
    sig.sa_handler = sig_handler;
    sigfillset(&sig.sa_mask);
    sig.sa_flags = 0;
    if (sigaction(SIGINT, &sig, NULL) < -1) {
        printf("Sigaction sigint error");
        exit(1);
    }
    if (sigaction(SIGSEGV, &sig, NULL) < -1) {
        printf("Sigaction sigsegv error");
        exit(1);
    }
}

void init_func(int *clients, int id, struct msg_buf *msg_buf) {
    clients[id] = (int)strtol(msg_buf->msg_text, NULL, 10);
    msg_buf->msg_type = REPLY;
    strcpy(msg_buf->msg_text, "Assigning ID");
    msg_buf->client_id = id;
}

void mirror_func(struct msg_buf *msg_buf) {
    int length = (int)strlen(msg_buf->msg_text);
    char tmp;
    for (int i = 0; i < length/2; ++i)
    {
        tmp = msg_buf->msg_text[i];
        msg_buf->msg_text[i] = msg_buf->msg_text[length-i-1];
        msg_buf->msg_text[length-i-1] = tmp;
    }
    msg_buf->msg_type = REPLY;
}

void calc_func(struct msg_buf *msg_buf) {
    char opr;
    char *tmp = malloc(10 * sizeof(char));
    int num1, num2, result;
    int length = (int)strlen(msg_buf->msg_text);
    int i = 5;
    while (msg_buf->msg_text[i] >= '0' && msg_buf->msg_text[i] <= '9' && i < length)
        i++;
    if (i == 0) {
        strcpy(msg_buf->msg_text, "Bad calc syntax1");
        msg_buf->msg_type = REPLY;
        return;
    }

    sprintf(tmp, "%.*s",i-5, msg_buf->msg_text+5);
    num1 = (int)strtol(tmp, NULL, 10);

    while (msg_buf->msg_text[i] == ' ' && i < length)
        i++;
    if (i == length) {
        strcpy(msg_buf->msg_text, "Bad calc syntax2");
        msg_buf->msg_type = REPLY;
        return;
    }

    opr = msg_buf->msg_text[i++];

    while (msg_buf->msg_text[i] == ' ' && i < length)
        i++;
    if (i == length || msg_buf->msg_text[i] < '0' || msg_buf->msg_text[i] > '9') {
        strcpy(msg_buf->msg_text, "Bad calc syntax3");
        msg_buf->msg_type = REPLY;
        return;
    }
    sprintf(tmp, "%.*s", length-i, msg_buf->msg_text + i);
    num2 = (int)strtol(tmp, NULL, 10);
    switch (opr)
    {
        case '+':
            result = num1 + num2;
            sprintf(msg_buf->msg_text, "%d", result);
            break;
        case '-':
            result = num1 - num2;
            sprintf(msg_buf->msg_text, "%d", result);
            break;
        case '/':
            if (num2) {
                printf("%d", num1/num2);
                result = (num1 / num2);
                sprintf(msg_buf->msg_text, "%d", (num1/num2));
            }
            else {
                strcpy(msg_buf->msg_text, "Can't divide by 0!");
                msg_buf->msg_type = REPLY;
                return;
            }
        case '*':
            result = num1 * num2;
            sprintf(msg_buf->msg_text, "%d", result);
            break;
        default:
            strcpy(msg_buf->msg_text, "Wrong sign!");
            msg_buf->msg_type = REPLY;
            return;
    }
    msg_buf->msg_type = REPLY;
}

void time_func(struct msg_buf *msg_buf) {
    time_t seconds = time(NULL);
    char *tmp;
    tmp = ctime(&seconds);
    strcpy(msg_buf->msg_text, tmp);
    msg_buf->msg_type = REPLY;
}

int main() {
    set_sigint();

    int server_q;
    if ((server_q = msgget(ftok("msgbuf.h", 0), S_IRWXU | S_IRWXG | S_IRWXO | IPC_CREAT)) == -1) {
        printf("Error while making queue in server\n");
        exit(1);
    }

    int *clients = malloc(100 * sizeof(int));
    int next_id = 0;

    struct msg_buf msg_buf;
    struct msqid_ds stat;
    int is_ended = 0;
    int msg_left = 1;
    while (msg_left) {
        if(!is_ended) {
            msgrcv(server_q, &msg_buf, sizeof(struct msg_buf) - sizeof(long), 0, 0);
        }
        switch (msg_buf.msg_type) {
            case INIT:
                init_func(clients, next_id, &msg_buf);
                next_id++;
                break;
            case MIRROR:
                mirror_func(&msg_buf);
                break;
            case CALC:
                calc_func(&msg_buf);
                break;
            case TIME:
                time_func(&msg_buf);
                break;
            case END:
                msgctl(server_q, IPC_STAT, &stat);
                msg_left = (int)stat.msg_qnum;
                is_ended = 1;
                continue;
            default:
                msg_buf.msg_type = REPLY;
                strcpy(msg_buf.msg_text, "Unknown command");
                break;
        }
        if (msgsnd(clients[msg_buf.client_id], &msg_buf, sizeof(struct msg_buf) - sizeof(long), 0) == -1) {
            printf("Msgsnd failed\n");
        }
        if (is_ended) msg_left--;
    }
    printf("Closing server...\n");
    msgctl(server_q, IPC_RMID, NULL);
    return 0;


}