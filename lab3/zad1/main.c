#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <ftw.h>
#include <string.h>
#include <time.h>
#include <zconf.h>
#include <sys/wait.h>

//function returns boolean value that depends of mode given as 2nd argument
int compare(int mode, time_t x, time_t y) {
    if (mode == 1)
        return difftime(x, y) > 0 ? 1 : 0;

    else if (mode == 0)
        return difftime(x, y) == 0 ? 1 : 0;

    else
        return difftime(y, x) > 0 ? 1 : 0;
}

//returns premissions of file given in stat buffer
char *get_permissions(const struct stat *buffer) {

    char *tmp = malloc(11 * sizeof(char));
    tmp = strcpy(tmp, "----------\0");

//& symbol - bitwise and operator
    if (buffer->st_mode & S_IRUSR)
        tmp[1] = 'r';
    if (buffer->st_mode & S_IWUSR)
        tmp[2] = 'w';
    if (buffer->st_mode & S_IXUSR)
        tmp[3] = 'x';
    if (buffer->st_mode & S_IRGRP)
        tmp[4] = 'r';
    if (buffer->st_mode & S_IWGRP)
        tmp[5] = 'w';
    if (buffer->st_mode & S_IXGRP)
        tmp[6] = 'x';
    if (buffer->st_mode & S_IROTH)
        tmp[7] = 'r';
    if (buffer->st_mode & S_IWOTH)
        tmp[8] = 'w';
    if (buffer->st_mode & S_IXOTH)
        tmp[9] = 'x';

    return tmp;
}

void find(char *path, int mode, time_t datetime) {

    if (path == NULL)
        return;

    DIR *directory = opendir(path);

    if (directory == NULL) {
        printf("Problem with opening directory %s.\n", path);
        return;
    }

    struct stat *buffer = malloc(sizeof(struct stat)); //info about file
    struct dirent *dir; //struct to readdir


    while ((dir = readdir(directory)) != 0) {


        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) //not reading . and .. dirs
            continue;

        char *tmp = malloc(PATH_MAX * sizeof(char)); //getting absolute path

        strcpy(tmp, path);
        strcat(tmp, "/");
        strcat(tmp, dir->d_name);

        char *absolute_path = realpath(tmp, NULL);
        stat(absolute_path, buffer);

        free(tmp);

        if (dir->d_type == DT_DIR) { //if its dir then go deeper
            pid_t pid;
            int status = 0;

            pid = fork();
            if (pid == -1) {
                printf("Failed fork!");
                exit(1);
            } else if (pid == 0) {
                find(absolute_path, mode, datetime);
            } else {
                while ((pid = waitpid(pid, &status, 0)) == 0);
                exit(EXIT_SUCCESS);
            }
            

        } else if (dir->d_type == DT_REG) { //if its reg compare modification date and print info
            if (compare(mode, datetime, buffer->st_mtime)) {

                char *permissions = get_permissions(buffer);
                printf("Absolute path: %s\n Size in bytes:%li\n Permissions: %s\n Modification date:%s\n",
                       absolute_path, buffer->st_size, permissions, ctime(&buffer->st_mtime));
                free(permissions);
            }
        } else { //rest of files
            printf("File type: %i\n", dir->d_type);
        }

        free(absolute_path);
    }
    free(buffer);
    closedir(directory);
}

int main(int argc, char *argv[]) {


    if (argc < 5) {
        printf("Not enough arguments!\n");
        return 1;
    }
    if (argc > 5) {
        printf("Too many arguments\n");
        return 1;
    }


    DIR *check = opendir(argv[1]); //check if directiory is good
    if (check == 0) {
        printf("Cannot open a directory.\n");
        return 1;
    }
    closedir(check);

    char *date = malloc(20 * sizeof(char)); //date conversion

    strcpy(date, argv[3]);
    strcat(date, " ");
    strcat(date, argv[4]);

    struct tm tmp;
    strptime(date, "%Y-%m-%d %H:%M:%S", &tmp);
    time_t time = mktime(&tmp);

    free(date);


    if (strcmp(argv[2], "<") == 0) {
        find(argv[1], 1, time);
    } else if (strcmp(argv[2], "=") == 0) {
        find(argv[1], 0, time);
    } else if (strcmp(argv[2], ">") == 0) {
        find(argv[1], -1, time);
    } else {
        printf("Wrong operator.\n");
        return 1;
    }

    return 0;
}