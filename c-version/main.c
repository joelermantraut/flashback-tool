#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    int task;
    int time;
    int date;
    int run;
} ArgsInfo;

typedef struct {
    char *task;
    char *date; // Date in format YYYY-MM-DD
    char *time; // Time in format HH:MM
} TaskInfo;

void print_help() {
    printf("Please provide the following arguments:\n");
    printf("--task <task>\n");
    printf("--time <time> or --date <date>\n");
}

int parse_arguments(int argc, char *argv[], ArgsInfo *args) {
    // if ((argc % 2) != 1) {
    //     printf("Invalid number of arguments\n");
    //     return 1;
    // }
    // This allows us to check for the next argument after the current one
    // without having to check for the end of the array

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--task") == 0) {
            args->task = i + 1;
        } else if (strcmp(argv[i], "--time") == 0) {
            args->time = i + 1;
        } else if (strcmp(argv[i], "--date") == 0) {
            args->date = i + 1;
        } else if (strcmp(argv[i], "--run") == 0) {
            args->run = 1;
            return 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
    }

    // Check if all the arguments were provided
    if (args->task == 0 || (args->time == 0 && args->date == 0)) {
        return 0;
    }
    
    return 1;
}

pthread_t* allocate_threads_list(pthread_t *threads, int n) {
    if (!threads) {
        threads = malloc(n * sizeof(pthread_t));
    } else {
        threads = realloc(threads, n * sizeof(pthread_t));
    }
    if (!threads) {
        printf("Error allocating memory for threads list\n");
        return NULL;
    }

    return threads;
}

TaskInfo* allocate_task_list(TaskInfo *tasks, int n) {
    if (!tasks) {
        tasks = malloc(n * sizeof(TaskInfo));
    } else {
        tasks = realloc(tasks, n * sizeof(TaskInfo));
    }
    if (!tasks) {
        printf("Error allocating memory for threads list\n");
        return NULL;
    }

    return tasks;
}

char* allocate_string(int n) {
    return malloc(n * sizeof(char));
}

void free_memory(void *ptr) {
    /* Frees memory and checks for errors */
    free(ptr);
    ptr = NULL;
}

FILE *open_file(char *filename, char *mode) {
    FILE *file = fopen(filename, mode);
    if (!file) {
        printf("Error opening file %s\n", filename);
        return NULL;
    }
    return file;
}

void close_file(FILE *file) {
    if (fclose(file) != 0) {
        printf("Error closing file\n");
    }
}

int lift_data_file(char *data_filename, TaskInfo *tasks) {
    FILE* data_file = open_file(data_filename, (char *) 'r');

    fseek(data_file, 0L, SEEK_END);
    int n_tasks = ftell(data_file) / sizeof(TaskInfo);

    tasks = allocate_task_list(tasks, n_tasks);

    fread(tasks, sizeof(TaskInfo), n_tasks, data_file);
    
    close_file(data_file);

    return n_tasks;
}

int compare_date_with_now(char *date) {
    return 1;
}

// Function executed by each thread
void* worker(void* arg) {
    int id = *(int*)arg;
    printf("Thread %d started\n", id);

    for (int i = 0; i < 10; i++) {
        sleep(1); // In seconds
    }

    printf("Thread %d finished\n", id);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    char *DATA_FILENAME = ".tasks";    
    TaskInfo *tasks = NULL;
    pthread_t *threads = NULL;
    int n_threads = 0;

    ArgsInfo args = {0};
    int success = parse_arguments(argc, argv, &args);

    if (!success) {
        print_help();
        return 1;
    }

    // if (!args.run) {
    //     // Add task to data file
    //     return 0;
    // }

    while (1) {
        int n_tasks = lift_data_file(DATA_FILENAME, tasks);
        for (int i = 0; i < n_tasks; i++) {
            int status = compare_date_with_now(tasks[i].date);
            if (!status) {
                n_threads++;
                threads = allocate_threads_list(threads, n_threads);
                pthread_create(&threads[i], NULL, worker, &i);
            } else {
                printf("Task %s is not valid\n", tasks[i].task);
            }
        }
        sleep(1);
    }

    free_memory(threads);

    return 0;
}