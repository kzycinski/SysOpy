#include <stdlib.h>
#include <fcntl.h>
#include <zconf.h>
#include <stdio.h>
#include <memory.h>
#include "library.h"


void system_copy(char *source_file, char *target_file, int records, int record_length){

    char *string = malloc(record_length * sizeof(char));

    int source_desc = open(source_file, O_RDONLY);
    int target_desc = open(target_file, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);

    for (int i = 0 ; i < records ; i++){
        if(read(source_desc, string, (size_t)record_length * sizeof(char)) != record_length){
            printf("Error while reading from %s!\n", source_file);
            return;
        }
        if(write(target_desc, string, (size_t)record_length * sizeof(char)) != record_length){
            printf("Error while writing to %s!\n",  target_file);
            return;
        }
    }
    close(source_desc);
    close(target_desc);
    free(string);
}

void system_sort(char *source_file, int records, int record_length) {

    int file_desc = open(source_file, O_RDWR);

    char *key = malloc(record_length * sizeof(char));
    char *tmp = malloc(record_length * sizeof(char));

    long int offset = (long int) (record_length * sizeof(char));

    int j;

    for(int i = 1 ; i < records ; i++) {

        lseek(file_desc, (i) * offset, SEEK_SET);

        if (read(file_desc, key, (size_t) record_length * sizeof(char)) != record_length) {
            printf("Error while reading from %s!\n", source_file);
            return;
        }

        lseek(file_desc, (-2) * offset, SEEK_CUR);

        if (read(file_desc, tmp, (size_t) record_length * sizeof(char)) != record_length) {
            printf("Error while reading from %s!\n", source_file);
            return;
        }

        j = i - 1;

        while (j > 0 && tmp[0] > key[0]) {
            if (write(file_desc, tmp, (size_t) record_length * sizeof(char)) != record_length) {
                printf("Error while reading from %s!\n", source_file);
                return;
            }

            lseek(file_desc, (-3) * offset, SEEK_CUR);

            if (read(file_desc, tmp, (size_t) record_length * sizeof(char)) != record_length) {
                printf("Error while reading from %s!\n", source_file);
                return;
            }

            j--;
        }
        if (tmp[0] > key[0] && j == 0 ){
            if (write(file_desc, tmp, (size_t) record_length * sizeof(char)) != record_length) {
                printf("Error while reading from %s!\n", source_file);
                return;
            }
            lseek(file_desc, (-2) * offset, SEEK_CUR);
        }

        if (write(file_desc, key, (size_t) record_length * sizeof(char)) != record_length) {
            printf("Error while reading from %s!\n", source_file);
            return;
        }
    }

    close(file_desc);
    free(tmp);
    free(key);
}

void generate(char *file_name, int records, int record_length){

    char *string = malloc((record_length) * sizeof(char));

    FILE *file = fopen(file_name, "w+");

    for(int i = 0 ; i < records ; i++){

        char *table = "ABCDEFGHIJKLMNOPRSTUWVXYZ";
        for (int i = 0 ; i < record_length; i++){
            string[i] = table[rand() % strlen(table)];
        }


        string[record_length-1] = 10;
        if(fwrite(string, sizeof(char), (size_t)record_length, file) != record_length) {
            printf("Error while writing to %s!\n", file_name);
            return;
        }
    }
    fclose(file);
    free(string);
}


void library_copy(char *source_file, char *target_file, int records, int record_length){

    char *string = malloc(record_length * sizeof(char));

    FILE *source = fopen(source_file, "r");
    FILE *target = fopen(target_file, "w+");

    for (int i = 0 ; i < records ; i++){
        if(fread(string, sizeof(char), (size_t)record_length, source) != record_length){
            printf("Error while reading from %s!\n", source_file);
            return;
        }
        if(fwrite(string, sizeof(char), (size_t)record_length, target) != record_length){
            printf("Error while writing to %s!\n",  target_file);
            return;
        }
    }
    fclose(source);
    fclose(target);
    free(string);
}

void library_sort(char *source_file, int records, int record_length){

    FILE *file = fopen(source_file, "r+");

    char *key = malloc(record_length * sizeof(char));
    char *tmp = malloc(record_length * sizeof(char));

    long int offset = (long int) (record_length * sizeof(char));

    int j;

    for(int i = 1 ; i < records ; i++) {

        fseek(file, (i) * offset, 0);

        if (fread(key, sizeof(char), (size_t) record_length, file) != record_length) {
            printf("Error while reading from %s!\n", source_file);
            return;
        }

        fseek(file, (-2) * offset, 1);

        if (fread(tmp, sizeof(char), (size_t) record_length, file) != record_length) {
            printf("Error while reading from %s!\n", source_file);
            return;
        }

        j = i - 1;

        while (j > 0 && tmp[0] > key[0]) {
            if (fwrite(tmp, sizeof(char), (size_t) record_length, file) != record_length) {
                printf("Error while reading from %s!\n", source_file);
                return;
            }

            fseek(file, (-3) * offset, 1);

            if (fread(tmp, sizeof(char), (size_t) record_length, file) != record_length) {
                printf("Error while reading from %s!\n", source_file);
                return;
            }

            j--;
        }
        if (tmp[0] > key[0] && j == 0 ){
            if (fwrite(tmp, sizeof(char), (size_t) record_length, file) != record_length) {
                printf("Error while reading from %s!\n", source_file);
                return;
            }
            fseek(file, (-2) * offset, 1);
        }

        if (fwrite(key, sizeof(char), (size_t) record_length, file) != record_length) {
            printf("Error while reading from %s!\n", source_file);
            return;
        }
    }

    fclose(file);
    free(tmp);
    free(key);
}