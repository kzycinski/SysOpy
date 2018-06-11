#define UNIX_PATH_MAX 108
#define CLIENT_MAX 10
#define PING_TIME 5

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

typedef struct message_t {
    enum message_type msg_type;
    char name[64];
    enum connect_type connect_type;
    int op;
    double val;
    int wrong_flag;
} message_t;

typedef struct operation_t {
    int operation;
    char operand;
    double arg1;
    double arg2;
} operation_t;

typedef struct Client {
    struct sockaddr* sockaddr;
    socklen_t socklen;
    enum connect_type connect_type;
    char *name;
    uint8_t is_inactive;
} Client;


