#define INIT 1
#define MIRROR 2
#define CALC 3
#define TIME 4
#define END 5
#define REPLY 6
#define UNKNOWN 7

#define MAX_LENGTH 50

struct msg_buf {
    long msg_type;
    char msg_text[MAX_LENGTH];
    pid_t client_pid;
    int client_id;
};

