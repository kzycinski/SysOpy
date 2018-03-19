#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>
#include <ftw.h>
#include <string.h>
#include <time.h>
#include <zconf.h>


int compare(int mode, time_t x, time_t y){
    if(mode == 1)
        return difftime(x,y) > 0 ? 1 : 0;

    else if(mode == 0)
        return difftime(x,y) == 0 ? 1 : 0;

    else
        return difftime(y,x) > 0 ? 1 : 0;
}


char *get_permissions(struct stat *buffer){

    char *tmp = malloc(11 * sizeof(char));
    tmp = strcpy(tmp, "----------\0");


    if(buffer -> st_mode & S_IRUSR)
        tmp[1] = 'r';
    if(buffer -> st_mode & S_IWUSR)
        tmp[2] = 'w';
    if(buffer -> st_mode & S_IXUSR)
        tmp[3] = 'x';
    if(buffer -> st_mode & S_IRGRP)
        tmp[4] = 'r';
    if(buffer -> st_mode & S_IWGRP)
        tmp[5] = 'w';
    if(buffer -> st_mode & S_IXGRP)
        tmp[6] = 'x';
    if(buffer -> st_mode & S_IROTH)
        tmp[7] = 'r';
    if(buffer -> st_mode & S_IWOTH)
        tmp[8] = 'w';
    if(buffer -> st_mode & S_IXOTH)
        tmp[9] = 'x';

    return tmp;
}

void find(char *path, int mode, time_t datetime){

    if (path == NULL)
        return;

    DIR *directory = opendir(path);

    if(directory == NULL) {
        printf("Problem with opening directory %s.\n", path);
        return;
    }

    struct stat *buffer = malloc(sizeof(struct stat));
    struct dirent *dir;
    char absolute_path[PATH_MAX];

    while((dir = readdir(directory)) != 0){


        if(strcmp(dir -> d_name, ".") == 0 || strcmp(dir -> d_name, "..") == 0)
            continue;

        strcpy(absolute_path, path);
        strcat(absolute_path, "/");
        strcat(absolute_path, dir->d_name);


        stat(absolute_path, buffer);

        if(dir -> d_type == DT_DIR){
            find(absolute_path, mode, datetime);
        }

        else if(dir -> d_type == DT_REG){
            if(compare(mode, datetime, buffer -> st_mtime)){

                char *permissions = get_permissions(buffer);
                printf("%s, %li, %s, %s\n", absolute_path, buffer -> st_size, permissions, ctime(&buffer -> st_mtime));
                free(permissions);
            }
        }
        else{
            printf("File type: %i\n", dir -> d_type);
        }
    }
    free(buffer);
    closedir(directory);
}

int main(int argc, char *argv[]) {

    if(argc<5){
        printf("Not enough arguments!\n");
        return 1;
    }

    DIR *check = opendir(argv[1]);
    if (check == 0) {
        printf("Cannot open a directory.\n");
        return 1;
    }
    closedir(check);

    char *date = malloc(20 * sizeof(char));

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