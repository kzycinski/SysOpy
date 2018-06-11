#ifndef ZAD1_CLUSTER_H
#define ZAD1_CLUSTER_H

#define UNIX_PATH_MAX 108
#define CLIENT_MAX 10
#define PING_TIME 5

typedef struct result_t {
    int wrong_flag;
    int operation;
    double value;
} result_t;

typedef struct operation_t {
    int operation;
    char operand;
    double arg1;
    double arg2;
} operation_t;

typedef struct Client {
    int fd;
    char *name;
    uint8_t is_inactive;
} Client;

typedef enum message_type {
    LOGIN,
    LOGOUT,
    SUCCESS,
    WRONG_SIZE,
    WRONG_NAME,
    REQUEST,
    RESULT,
    PING,
    PONG,
} message_type;

typedef enum connect_type {
    LOCAL,
    WEB
} connect_type;

#endif //ZAD1_CLUSTER_H