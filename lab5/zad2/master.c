#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Wrong arguments\n");
        exit(1);
    }

    mkfifo(argv[1], S_IWUSR | S_IRUSR);

    FILE *pipe = fopen(argv[1], "r");

    if (pipe == NULL) {
        printf("Pipe error\n");
        exit(1);
    }

    char *buf = (char *) calloc(50, sizeof(char));

    while (1) {
        fread(buf, 1, 1, pipe);
        printf("%s", buf);
    }
}