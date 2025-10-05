#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define STRING_SIZE 80

typedef struct {
    int task;
    int time;
    int date;
    int run;
} ArgsInfo;

typedef struct {
    char task[120];
    char date[10]; // Date in format YYYY-MM-DD
    char time[5]; // Time in format HH:MM
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

TaskInfo *lift_data_file(char *data_filename, int *n_tasks) {
    FILE* data_file = open_file(data_filename, (char *) "rb");

    fseek(data_file, 0L, SEEK_END);
    *n_tasks = ftell(data_file) / sizeof(TaskInfo);
    fseek(data_file, 0L, SEEK_SET);

    TaskInfo *tasks = NULL;
    tasks = allocate_task_list(tasks, *n_tasks);

    fread(tasks, sizeof(TaskInfo), *n_tasks, data_file);

    close_file(data_file);

    return tasks;
}

int compare_date_with_now(char *date) {
    return 1;
}

void add_task(char *data_filename, char *task, char *date) {
    TaskInfo task_info;

    strcpy(task_info.task, task);
    strcpy(task_info.date, date);
    strcpy(task_info.time, "00:00");

    const char *mode = (access(data_filename, F_OK) == 0) ? "ab" : "wb";

    FILE* data_file = open_file(data_filename, mode);
    fwrite(&task_info, sizeof(char), sizeof(TaskInfo), data_file);
    fclose(data_file);
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
    int n_tasks = 0;

    ArgsInfo args = {0};
    int success = parse_arguments(argc, argv, &args);

    if (!success) {
        print_help();
        return 1;
    }

    if (!args.run) {
        printf("Adding task %s\n", argv[args.task]);
        // if (args.date) strcpy(date, argv[args.date]);
        // else strcpy(date, "Testing");
        add_task(DATA_FILENAME, argv[args.task], argv[args.date]);
        return 0;
    }

    while (1) {
        tasks = lift_data_file(DATA_FILENAME, &n_tasks);
        for (int i = 0; i < n_tasks; i++) {
            int status = compare_date_with_now(tasks[i].date);
            if (!status) {
                n_threads++;
                threads = allocate_threads_list(threads, n_threads);
                pthread_create(&threads[i], NULL, worker, &i);
                printf("Task %s is being executed\n", tasks[i].task);
            } else {
                printf("Task %s is not valid\n", tasks[i].task);
            }
        }
        sleep(1);
    }

    free_memory(threads);

    return 0;
}