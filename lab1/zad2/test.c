//
// Created by krystian on 12.03.18.
//dlopen, dlsym,

#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <zconf.h>
#include <sys/times.h>
#include <memory.h>
#include <time.h>

char *operation_print;
int continued = 0;

char *generate_rand_block(int length){
    if (length < 1 || length >= STATIC_BLOCK_SIZE) return NULL;
    char *table = "1234567890abcdefghijklmnoprstuwvxyzABCDEFGHIJKLMNOPRSTUWVXYZ!@#$%^&*()";
    char *block = malloc (length * sizeof(char));

    for (int i = 0 ; i < length-1 ; i++){
        block[i] = table[rand() % strlen(table)];
    }
    block[length] = '\0';
    return block;
}
void delete_blocks(struct array_struct *arr_struct, int start_index, int number_of_blocks){
    for (int i = start_index ; i < (start_index + number_of_blocks) ; i++){
        delete_block(arr_struct,i);
    }
}
void add_blocks(struct array_struct *arr_struct, int start_index, int number_of_blocks, int block_size){
    for (int i = start_index ; i < start_index + number_of_blocks ; i++){
        char *tmp = generate_rand_block(block_size);
        add_block(arr_struct, i, tmp);
    }
}
double calculate_timediff(clock_t start, clock_t end){
    return (double)((end - start) / sysconf(_SC_CLK_TCK));
}
void print_time(clock_t start_time, clock_t end_time, struct tms tms_start, struct tms tms_end, FILE *file){
    printf("Real time: %lf    ", calculate_timediff(start_time, end_time));
    printf("User time: %lf    ", calculate_timediff(tms_start.tms_utime, tms_end.tms_utime));
    printf("System time: %lf    \n", calculate_timediff(tms_start.tms_stime, tms_end.tms_stime));
    fprintf(file, "Real time: %lf    ", calculate_timediff(start_time, end_time));
    fprintf(file, "User time: %lf    ", calculate_timediff(tms_start.tms_utime, tms_end.tms_utime));
    fprintf(file, "System time: %lf    \n", calculate_timediff(tms_start.tms_stime, tms_end.tms_stime));
}
void make_operation(struct array_struct *arrstruct, char *operation, int parameter, int block_length){
    if(strcmp(operation, "find") == 0){
        operation_print = "Finding closest block:\n";
        findBlock(arrstruct,parameter);
        continued = 1;
        return;
    }
    else if(strcmp(operation, "dta") == 0){
        operation_print = "Deleting then adding:\n";
        delete_blocks(arrstruct, 0, parameter);
        add_blocks(arrstruct, 0, parameter,block_length);
        continued = 1;
    }
    else if(strcmp(operation, "ada") == 0){
        operation_print = "Alternately deleting and adding:\n";
        for(int i = 0 ; i < parameter ; i++){
            delete_block(arrstruct, i);
            add_block(arrstruct, i, generate_rand_block(block_length));
            continued = 1;
        }
    }
    else {
        printf("Wrong argument, choose one of the following: \n find - finds block with closest char sum \n ");
        printf("dta - delete then add given number of blocks  \n ada - alternately delete and add blocks \n ");
    }
}
int main(int argc, char *argv[]){


    if (argc < 4) {
        printf("Not enough arguments, please input array size, block length and allocation type.");
        return 0;
    }
    if (argc > 10) {
        printf("Too many commands, maximum is 3.");
        return 0;
    }

    int array_size = (int) strtol(argv[1], NULL, 10);
    int block_length = (int) strtol(argv[2], NULL, 10);
    int is_static = (int) strtol(argv[3], NULL, 10);

    FILE *file = fopen("raport2.txt", "a");
    if (file == NULL){
        printf("Cannot open the file.");
        return 1;
    }

    srand(time(NULL));

    printf("Array Size: %d   Block Length: %d   Allocation: %d\n", array_size, block_length, is_static);
    fprintf(file, "Array Size: %d   Block Length: %d   Allocation: %d\n", array_size, block_length, is_static);

    clock_t *real_time = (clock_t*) malloc(8 * sizeof(clock_t));
    struct tms *tms_time[8];
    for (int i = 0; i < 8; i++) {
        tms_time[i] = malloc(sizeof(struct tms*));
    }

    real_time[0] = times(tms_time[0]);

    for(int i = 0 ; i < 10000 ; i++) {
        create_struct(array_size, block_length, is_static);
    }
    real_time[1] = times(tms_time[1]);

    struct array_struct *arrstruct = create_struct(array_size, block_length, is_static);

    printf("Create array:\n");
    fprintf(file, "Create array:\n");
    print_time(real_time[0], real_time[1], *tms_time[0], *tms_time[1], file);

    real_time[2] = times(tms_time[2]);

    add_blocks(arrstruct,0,arrstruct -> blocks, arrstruct -> block_size);

    real_time[3] = times(tms_time[3]);

    printf("Fill array:\n");
    fprintf(file, "Fill array:\n");
    print_time(real_time[2], real_time[3], *tms_time[2], *tms_time[3], file);

    if(argc > 5) {
        real_time[4] = times(tms_time[4]);
        make_operation(arrstruct, argv[4], (int) strtol(argv[5], NULL, 10), block_length);
        real_time[5] = times(tms_time[5]);

            printf("%s", operation_print);
            fprintf(file, "%s", operation_print);
            print_time(real_time[4], real_time[5], *tms_time[4], *tms_time[5], file);
    }
    if(argc > 7) {
        real_time[6] = times(tms_time[6]);
        make_operation(arrstruct, argv[6], (int) strtol(argv[7], NULL, 10), block_length);
        real_time[7] = times(tms_time[7]);

            printf("%s", operation_print);
            fprintf(file, "%s", operation_print);
            print_time(real_time[6], real_time[7], *tms_time[6], *tms_time[7], file);
    }

    fclose(file);


}

