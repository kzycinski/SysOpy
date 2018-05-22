#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

int *input_file;
int *output_file;
double *filter;
int img_width;
int img_height;
int c;
char shit[2];
int max_value;
int threads_no;
int pixels_count;


void print_info() {
    printf("Please use following pattern to run the progran:\n");
    printf("./filter no_threads source_file filter output_file\n");
}

double calc_realtime(struct timeval start, struct timeval end) {
    return (((double) end.tv_sec * 1000000 + end.tv_usec) - ((double) start.tv_sec * 1000000 + start.tv_usec)) /
           1000000;
}

double calc_usertime(struct rusage *r_usage) {
    return (((double) (r_usage->ru_utime.tv_sec) * 1000000) + r_usage->ru_utime.tv_usec) / 1000000;
}

double calc_systemtime(struct rusage *r_usage) {
    return (((double) (r_usage->ru_stime.tv_sec) * 1000000) + r_usage->ru_stime.tv_usec) / 1000000;
}

void load_files(char **argv) {
    FILE *file;
    if ((file = fopen(argv[2], "r")) == NULL) {
        printf("Cannot open the input file\n");
        exit(1);
    }

    fscanf(file, "%s %d %d %d", shit, &img_width, &img_height, &max_value);

    input_file = malloc(img_width * img_height * sizeof(*input_file));
    output_file = malloc(img_width * img_height * sizeof(*output_file));

    for (int i = 0; i < img_height; ++i) {
        for (int j = 0; j < img_width; ++j) {
            fscanf(file, "%d", &input_file[i * img_width + j]);
        }
    }

    fclose(file);

    if ((file = fopen(argv[3], "r")) == NULL) {
        printf("Cannot open the filter file\n");
        exit(1);
    }

    fscanf(file, "%d", &c);

    filter = malloc(c * c * sizeof(*filter));

    for (int i = 0; i < c; ++i) {
        for (int j = 0; j < c; ++j) {
            fscanf(file, "%lf", &filter[i * c + j]);
        }
    }


    fclose(file);
}


void *thread_handler(void *arg_void) {
    int x;
    int y;
    double sum;
    int a;
    int b;

    int thread_no = *(int *) arg_void;

    int start_thread = thread_no * pixels_count;
    int end_thread = min(start_thread + pixels_count, img_width * img_height);

    for (int i = start_thread; i < end_thread; ++i) {
        x = i / img_width;
        y = i % img_width;

        sum = 0;
        for (int i = 0; i < c; ++i) {
            for (int j = 0; j < c; ++j) {
                a = min(img_height - 1, max(1, x - (int) ceil(c / 2) + i));
                b = min(img_width - 1, max(1, y - (int) ceil(c / 2) + j));

                sum += input_file[a * img_width + b] * filter[i * c + j];
            }
        }

        output_file[i] = (int) round(sum);
    }

    return NULL;
}

void save_file(char *path) {
    FILE *image;
    if ((image = fopen(path, "w")) == NULL) {
        printf("Cannot open the result file\n");
        exit(1);
    }

    fprintf(image, "%s\n%d %d\n%d\n", shit, img_width, img_height, max_value);

    for (int i = 0; i < img_height; ++i) {
        for (int j = 0; j < img_width; ++j) {
            fprintf(image, "%d ", output_file[i * img_width + j]);
        }
        fprintf(image, "\n");
    }

    fclose(image);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Wrong args\n");
        print_info();
        exit(1);
    }
    struct timeval start, end;
    struct rusage *r_usage = malloc(sizeof(struct rusage));
    load_files(argv);

    sscanf(argv[1], "%d", &threads_no);

    pixels_count = (int) round(img_width * img_height / threads_no);

    if (threads_no > sysconf(_SC_NPROCESSORS_ONLN)) {
        printf("Warning: this system has only %ld processors available\n",
               sysconf(_SC_NPROCESSORS_ONLN));
    }

    gettimeofday(&start, NULL);

    pthread_t threads[threads_no];

    for (int i = 0; i < threads_no; ++i) {
        int *arg_i = malloc(sizeof(int));
        *arg_i = i;
        pthread_create(threads + i, NULL, thread_handler, arg_i);
    }

    for (int i = 0; i < threads_no; ++i) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);
    getrusage(RUSAGE_SELF, r_usage);

    save_file(argv[4]);

    printf("Threads: %d\tReal time: %fs\t User time: %fs\t System time: %fs\n", threads_no,
           calc_realtime(start, end),
           calc_usertime(r_usage),
           calc_systemtime(r_usage));

    FILE *file = fopen("Times.txt", "a");

    fprintf(file, "Threads: %d\tReal time: %fs\t User time: %fs\t System time: %fs\n", threads_no,
            calc_realtime(start, end), calc_usertime(r_usage),
            calc_systemtime(r_usage));
    fclose(file);
    free(file);


    free(input_file);
    free(output_file);
    free(filter);

    return 0;
}