#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include "library.h"
#include <sys/time.h>
#include <sys/resource.h>

//calculate system time in function call
double calculate_system_timediff(struct rusage *r_usage){
    return (((double)(r_usage[0].ru_stime.tv_sec) * 1000000) + r_usage[0].ru_stime.tv_usec) / 1000000;
}

//calculate user time in function call
double calculate_user_timediff(struct rusage *r_usage){
    return (((double)(r_usage[0].ru_utime.tv_sec) * 1000000) + r_usage[0].ru_utime.tv_usec) / 1000000;
}



//prints time to console and to file
void print_time(struct rusage *r_usage, FILE *file){
    printf("User time: %f\t", calculate_user_timediff(r_usage));
    printf("System time: %f\n", calculate_system_timediff(r_usage));
    fprintf(file, "User time: %f\t", calculate_user_timediff(r_usage));
    fprintf(file, "System time: %f\n", calculate_system_timediff(r_usage));
}

int main (int argc, char *argv[]) {

    FILE *file = fopen("wynik1.txt", "a");
    srand(time(NULL));
    if (argc < 5) {
        printf("Not enough arguments!\n");
        fclose(file);
        return 0;
    }

    if (argc > 7) {
        printf("Too many arguments!\n");
        fclose(file);
        return 0;
    }

    struct rusage *r_usage = malloc(sizeof(*r_usage));

    if(strcmp(argv[1], "generate") == 0 && argc == 5){

        int records = (int)strtol(argv[3], NULL, 10);
        int record_length = (int)strtol(argv[4], NULL, 10);

        printf("Generating file with %d records, each %d bytes long.\n", records, record_length);
        fprintf(file, "Generating file with %d records, each %d bytes long.\n", records, record_length);

        generate(argv[2], records, record_length);

        getrusage(RUSAGE_SELF, &r_usage[0]);
        print_time(r_usage,file);

        fclose(file);
        return 0;
    }
    else if(strcmp(argv[1], "sort") == 0 && argc == 6){

        int records = (int)strtol(argv[3], NULL, 10);
        int record_length = (int)strtol(argv[4], NULL, 10);

        if(strcmp(argv[5], "sys") == 0){
            printf("Sorting file with %d records, each %d bytes long, using system functions.\n", records, record_length);
            fprintf(file, "Sorting file with %d records, each %d bytes long, using system functions.\n", records, record_length);

            system_sort(argv[2], records, record_length);

            getrusage(RUSAGE_SELF, &r_usage[0]);
            print_time(r_usage,file);

            fclose(file);
            return 0;
        }

        else if(strcmp(argv[5], "lib") == 0){
            printf("Sorting file with %d records, each %d bytes long, using library functions.\n", records, record_length);
            fprintf(file, "Sorting file with %d records, each %d bytes long, using library functions.\n", records, record_length);

            library_sort(argv[2], records, record_length);

            getrusage(RUSAGE_SELF, &r_usage[0]);
            print_time(r_usage,file);

            fclose(file);
            return 0;
        }

        else {
            printf("Last argument invalid, available options: sys, lib.\n");

            fclose(file);
            return 0;
        }
    }
    else if(strcmp(argv[1], "copy") == 0 && argc == 7){


        int records = (int)strtol(argv[4], NULL, 10);
        int record_length = (int)strtol(argv[5], NULL, 10);
        if(strcmp(argv[6], "sys") == 0){
            printf("Copying file with %d records, each %d bytes long, using system functions.\n", records, record_length);
            fprintf(file, "Copying file with %d records, each %d bytes long, using system functions.\n", records, record_length);

            system_copy(argv[2], argv[3], records, record_length);

            getrusage(RUSAGE_SELF, &r_usage[0]);
            print_time(r_usage,file);

            fclose(file);
            return 0;
        }

        else if(strcmp(argv[6], "lib") == 0){
            printf("Copying file with %d records, each %d bytes long, using library functions.\n", records, record_length);
            fprintf(file, "Copying file with %d records, each %d bytes long, using library functions.\n", records, record_length);

            library_copy(argv[2], argv[3], records, record_length);

            getrusage(RUSAGE_SELF, &r_usage[0]);
            print_time(r_usage,file);

            fclose(file);
            return 0;
        }

        else {
            printf("Last argument invalid, available options: sys, lib.\n");

            fclose(file);
            return 0;
        }
    }
    else{
        printf("Enter valid arguments!\n");

        fclose(file);
        return 0;
    }

}























