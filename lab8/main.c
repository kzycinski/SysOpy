#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <math.h>
#include <zconf.h>


int **input_file;
int **output_file;
float **filter;
int img_width;
int img_height;
int pixel_max;
int c;
int threads;

void print_info() {
    printf("Please use following pattern to run the progran:\n");
    printf("./filter no_threads source_file filter output_file\n");
}

double calc_realtime(struct timeval start, struct timeval end) {
    return (((double) end.tv_sec * 1000000 + end.tv_usec) - ((double) start.tv_sec * 1000000 + start.tv_usec)) /
           1000000;
}

double calc_usertime(struct rusage *r_usage) {
    return (((float) (r_usage->ru_utime.tv_sec) * 1000000) + r_usage->ru_utime.tv_usec) / 1000000;
}

double calc_systemtime(struct rusage *r_usage) {
    return (((float) (r_usage->ru_stime.tv_sec) * 1000000) + r_usage->ru_stime.tv_usec) / 1000000;
}

int max(double a, double b) {
    return (int) (a > b ? a : b);
}

int min(double a, double b) {
    return (int) (a < b ? a : b);
}

void *thread_handler(void *argv) {
    int thread_no = *(int *) argv;

    int start_thread = img_width * thread_no / threads;
    int end_thread = img_width * (thread_no + 1) / threads;

    for (int i = start_thread; i < end_thread; i++) {

        for (int j = 0; j < img_height; j++) {

            double s_xy = 0;

            for (int k = 0; k < c; k++) {
                for (int m = 0; m < c; m++) {
                    //s_xy += (input_file[max(1, i - ceil(c / 2) + k)][max(1, j - ceil(c) + m)] * filter[k][m]);
                    s_xy += input_file[min(img_width - 1, max(0, i - ceil(c / 2) + k))]
                            [min(img_height - 1, max(0, j - ceil(c / 2) + m))] * filter[k][m];
                }
            }

            output_file[i][j] = (int) round(s_xy);
        }
    }

    return NULL;
}


void scan_files(char **argv) {
    char *buff = malloc(20 * sizeof(char));
    FILE *file;
    file = fopen(argv[2], "r");
    if (file == NULL) {
        printf("Cannot open the input file\n");
        exit(1);
    }

    fscanf(file, "%s %d %d %d", buff, &img_width, &img_height, &pixel_max);

    free(buff);

    input_file = malloc(img_width * sizeof(int *));
    output_file = malloc(img_width * sizeof(int *));

    if (input_file == NULL || output_file == NULL) {
        printf("Cannot allocate memory for i/o\n");
        exit(1);
    }

    int tmp;

    for (int i = 0; i < img_width; i++) {
        input_file[i] = malloc(img_height * sizeof(int));
        output_file[i] = malloc(img_height * sizeof(int));

        if (input_file[i] == NULL || output_file[i] == NULL) {
            printf("Cannot allocate memory for i/o\n");
            exit(1);
        }

        for (int j = 0; j < img_height; j++) {
            fscanf(file, "%d", &tmp);
            input_file[i][j] = tmp;
            output_file[i][j] = 0;
        }
    }

    fclose(file);

    file = fopen(argv[3], "r");

    if (file == NULL) {
        printf("Cannot open the filter file\n");
        exit(1);
    }

    fscanf(file, "%d", &c);

    filter = malloc(c * sizeof(float *));
    if (filter == NULL) {
        printf("Cannot allocate memory for filter i/o\n");
        exit(1);
    }

    float tmp2;

    for (int i = 0; i < c; i++) {
        filter[i] = malloc(c * sizeof(float));
        if (filter[i] == NULL) {
            printf("Cannot allocate memory for filter i/o\n");
            exit(1);
        }

        for (int j = 0; j < c; j++) {
            fscanf(file, "%f", &tmp2);
            filter[i][j] = tmp2;
        }
    }

    fclose(file);
    free(file);
}

void save_image(char *filename) {
    FILE *file;
    file = fopen(filename, "w");

    if (file == NULL) {
        printf("Cannot open the result file\n");
        exit(1);
    }

    fprintf(file, "P2\n%d %d\n255\n", img_width, img_height);

    for (int i = 0; i < img_width; i++) {
        for (int j = 0; j < img_height; j++) {
            fprintf(file, "%d ", output_file[i][j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    free(file);
}

int main(int argc, char **argv) {
    if (argc != 5) {
        printf("Wrong args\n");
        print_info();
        exit(1);
    }

    scan_files(argv);

    threads = (int) strtol(argv[1], NULL, 10);

    if (threads > sysconf(_SC_NPROCESSORS_ONLN)) {
        printf("Warning: this system has only %ld processors available\n",
               sysconf(_SC_NPROCESSORS_ONLN));
    }

    pthread_t *thread = malloc(threads * sizeof(pthread_t));
    if (thread == NULL) {
        printf("Cannot allocate memory for threads\n");
        exit(1);
    }
    struct timeval start, end;
    struct rusage *r_usage = malloc(sizeof(struct rusage));

    gettimeofday(&start, NULL);

    for (int i = 0; i < threads; i++) {
        int *arg_i = malloc(sizeof(int));
        *arg_i = i;
        pthread_create(&thread[i], NULL, thread_handler, arg_i);
    }

    for (int i = 0; i < threads; i++) {
        void *tmp;
        pthread_join(thread[i], &tmp);
    }

    gettimeofday(&end, NULL);
    getrusage(RUSAGE_SELF, r_usage);


    save_image(argv[4]);


    printf("Threads: %d\tReal time: %fs\t User time: %fs\t System time: %fs\n", threads, calc_realtime(start, end),
           calc_usertime(r_usage),
           calc_systemtime(r_usage));

    FILE *file = fopen("Times.txt", "a");

    fprintf(file, "Threads: %d\tReal time: %fs\t User time: %fs\t System time: %fs\n", threads,
            calc_realtime(start, end), calc_usertime(r_usage),
            calc_systemtime(r_usage));
    fclose(file);
    free(file);

    return 0;
}
