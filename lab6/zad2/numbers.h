#define INIT 1
#define MIRROR 2
#define CALC 3
#define TIME 4
#define END 5
#define UNKNOWN 7
#define CLOSED 8


#define MSG_SIZE sizeof(Msg)
#define MAX_MQSIZE 9
#define MAX_LENGTH 100
#define MAX_CLIENTS 100


#define SERVER_NAME "/serv"
#define MAX_MSG 1000
#define MAX_CONT_SIZE 4096
typedef struct msg{
    long msg_type;
    pid_t client_pid;
    char msg_text[MAX_CONT_SIZE];
} msg;



typedef struct Msg{
    long mtype;
    pid_t senderPID;
    char cont[MAX_CONT_SIZE];
} Msg;





