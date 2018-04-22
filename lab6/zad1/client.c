#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <zconf.h>
#include <memory.h>

#include "msg_buf.h"

void sig_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nShutting down\n");
        exit(EXIT_SUCCESS);
    } else if (sig == SIGSEGV)
        printf("Wtf");
}

void set_sigint() {
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

void init(struct msg_buf *msg_buf, int server_q, int client_q) {
    msg_buf->msg_type = INIT;
    msg_buf->client_pid = getpid();
    sprintf(msg_buf->msg_text, "%d", client_q);
    msg_buf->client_id = 0;

    if (msgsnd(server_q, msg_buf, sizeof(struct msg_buf) - sizeof(long), 0) == -1) {
        printf("Client- INIT failed\n");
        exit(1);
    }

    msgrcv(client_q, msg_buf, sizeof(struct msg_buf) - sizeof(long), 0, 0);
    printf("%s: %d\n", msg_buf->msg_text, msg_buf->client_id);
}

void parse_line(char *buf, struct msg_buf *msg_buf) {
    strcpy(msg_buf->msg_text, buf);
    char *type = strtok(buf, " \n");
    if (strcmp(type, "MIRROR") == 0) {
        msg_buf->msg_type = MIRROR;
        strcpy(msg_buf->msg_text, strtok(NULL, " \n"));
    } else if (strcmp(type, "CALC") == 0) {
        msg_buf->msg_type = CALC;
    } else if (strcmp(type, "TIME") == 0) {
        msg_buf->msg_type = TIME;
        strcpy(msg_buf->msg_text, "");
    } else if (strcmp(type, "END") == 0) {
        msg_buf->msg_type = END;
        strcpy(msg_buf->msg_text, "end");
    } else {
        msg_buf->msg_type = UNKNOWN;
        strcpy(msg_buf->msg_text, "");
    }
}

void file_mode(FILE *file, struct msg_buf *msg_buf, int server_q, int client_q) {
    char *buf = (char *) malloc(100 * sizeof(char));
    while (fgets(buf, 100, file) != NULL) {

        if (msg_buf->msg_type == CLOSED) {
            printf("No response from server");
            exit(1);
        }
        printf("%s", buf);
        if (strlen(buf) > MAX_LENGTH) {
            printf("Too long command\n");
            continue;
        }
        parse_line(buf, msg_buf);
        if (msgsnd(server_q, msg_buf, sizeof(struct msg_buf) - sizeof(long), 0) == -1) {
            printf("Client-send failed\n");
        }
        if (strcmp(msg_buf->msg_text, "end") == 0)
            return;

        if (msgrcv(client_q, msg_buf, sizeof(struct msg_buf) - sizeof(long), 0, 0) == -1) {
            printf("No response from server");
            exit(1);
        }

        printf("%s\n", msg_buf->msg_text);
    }
    free(buf);
}

void input_mode(FILE *file, struct msg_buf *msg_buf, int server_q, int client_q) {
    char *buf = (char *) malloc(100 * sizeof(char));
    int timeToEnd = 0;

    while (fgets(buf, 100, file) != NULL) {
        if (strlen(buf) > MAX_LENGTH) {
            printf("CToo long command\n");
            continue;
        }

        if (strcmp(buf, "END\n") == 0)
            timeToEnd = 1;

        parse_line(buf, msg_buf);

        if (msgsnd(server_q, msg_buf, sizeof(struct msg_buf) - sizeof(long), 0) == -1) {
            printf("Client-send failed\n");
            raise(SIGINT);
        }

        if (timeToEnd)
            break;
        if (msgrcv(client_q, msg_buf, sizeof(struct msg_buf) - sizeof(long), 0, 0) == -1) {
            printf("No response from server");
            exit(1);
        }
        if (msg_buf->msg_type == CLOSED) {
            printf("No response from server\n");
            exit(1);
        }
        printf("%s\n", msg_buf->msg_text);
    }
}

int main(int argc, char **argv) {
    if (argc > 2) {
        printf("Too many arguments!\n");
        exit(1);
    }
    set_sigint();

    int server_q, client_q;
    if ((server_q = msgget(ftok("msgbuf.h", 0), 0)) == -1) {
        printf("Error with serverq(client)\n");
        exit(1);
    }

    if ((client_q = msgget(IPC_PRIVATE, S_IRWXU | S_IRWXG | S_IRWXO)) == -1) {
        printf("Error with clientq\n");
        exit(1);
    }

    struct msg_buf msg_buf;

    init(&msg_buf, server_q, client_q);

    FILE *file;

    if (argc == 2) {
        if ((file = fopen(argv[1], "r")) == NULL) {
            printf("File opening error\n");
            exit(1);
        }
        file_mode(file, &msg_buf, server_q, client_q);
    } else {
        file = stdin;
        input_mode(file, &msg_buf, server_q, client_q);
    }

    if (msgctl(client_q, IPC_RMID, NULL) == 0) {
        printf("Clientq success\n");
    }

    return 0;
}