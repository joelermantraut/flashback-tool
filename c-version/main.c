#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>
#include <time.h>

#include "pipecomm.h"

// Includes

#define STRING_SIZE 80
#define PIPE_NAME "FlashBackPipe"

// Definitions

typedef struct {
    int task;
    int time;
    int date;
    int run;
} ArgsInfo;

typedef struct {
    int year;
    int month;
    int day;
} Date;

typedef struct {
    char task[120];
    Date date;
    time_t time;
} TaskInfo;

// Structures

TaskInfo *tasks = NULL;
int n_tasks = 0;

// Variables

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

// Arguments parsing

void show_task(char *task) {
    printf("Task %s executed\n", task);
}

// Notification management

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

TaskInfo *free_task_from_memory(TaskInfo *tasks, int index, int n_tasks) {
    for (int i = index; i < n_tasks - 1; i++) {
        tasks[i] = tasks[i + 1];
    }
    return realloc(tasks, (n_tasks - 1) * sizeof(TaskInfo));
}

void free_memory(void *ptr) {
    /* Frees memory and checks for errors */
    free(ptr);
    ptr = NULL;
}

// Memory management

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
    if (access(data_filename, F_OK)) {
        *n_tasks = 0;
        return NULL;
    }

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

// File management

int compare_datetime_with_now(Date date_struct, time_t time_struct) {
    time_t now = time(NULL);

    if (difftime(time_struct, now) <= 0) return 1;
    return 0;
}

time_t convert_str_to_time(char *time_str) {
    /*
    Convert string in minutes.
    */
    int minutes = atoi(time_str);

    time_t now = time(NULL);
    time_t future = now + minutes * 60;

    return future;
}

Date convert_str_to_date(char *date_str) {
    /*
    Convert string in format MM/DD/YYYY to Date struct.
    */
    Date date = {0};
    const char *delim = "/";
    char *segment;

    segment = strtok(date_str, ":");
    date.month = atoi(segment);
    segment = strtok(NULL, ":");
    date.day = atoi(segment);
    segment = strtok(NULL, ":");
    date.year = atoi(segment);

    return date;
}

// Date management

TaskInfo send_task(char *data_filename, char *task, char *date, char* time) {
    TaskInfo task_info;

    time_t time_struct = convert_str_to_time(time);
    Date date_struct = convert_str_to_date(date);

    strcpy(task_info.task, task);
    task_info.date = date_struct;
    task_info.time = time_struct;

    const char *mode = (access(data_filename, F_OK) == 0) ? "ab" : "wb";

    // FILE* data_file = open_file(data_filename, mode);
    // fwrite(&task_info, sizeof(char), sizeof(TaskInfo), data_file);
    // fclose(data_file);
    
    return task_info;
}

TaskInfo *append_task(TaskInfo *tasks, int *n_tasks, TaskInfo task) {
    tasks = allocate_task_list(tasks, *n_tasks);
    tasks[*n_tasks] = task;
    *n_tasks++;

    return tasks;
}

TaskInfo *check_tasks_status(TaskInfo *tasks, int *n_tasks) {
    if (*n_tasks == 0 || tasks == NULL)
        return tasks;

    int i = 0;
    while (i < *n_tasks) {
        int status = compare_datetime_with_now(tasks[i].date, tasks[i].time);
        // status == 0 => task after now
        // status == 1 => task before now or now

        printf("Checking task %s\n", tasks[i].task);

        if (status) {
            show_task(tasks[i].task);
            printf("Task %s is not valid (removing)\n", tasks[i].task);

            tasks = free_task_from_memory(tasks, i, *n_tasks);
            (*n_tasks)--;

            continue;
        }

        i++; // Solo avanzamos si no se eliminó
    }

    return tasks;
}

void on_new_task(const char *message) {
    TaskInfo *t = (TaskInfo *) message;
    printf("Appending task %s\n", t->task);

    append_task(tasks, &n_tasks, *t);
}

// Task management

int main(int argc, char *argv[]) {
    char *DATA_FILENAME = ".tasks";

    ArgsInfo args = {0};
    int success = parse_arguments(argc, argv, &args);

    if (!success) {
        print_help();
        return 1;
    }

    if (!args.run) {
        printf("Adding task %s\n", argv[args.task]);
        TaskInfo task = send_task(DATA_FILENAME, argv[args.task], argv[args.date], argv[args.time]);
        pipecomm_send_struct(PIPE_NAME, &task, sizeof(TaskInfo));
        return 0;
    }

    tasks = lift_data_file(DATA_FILENAME, &n_tasks);
    while (1) {
        tasks = check_tasks_status(tasks, &n_tasks);

        int status = pipecomm_start_server(PIPE_NAME, on_new_task);
        if (!status) {
            printf("Waiting for task...\n");
        } else {
            printf("Error starting server\n");
            return -1;
        }

        sleep(1);
    }

    free_memory(tasks);

    return 0;
}