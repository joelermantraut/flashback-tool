// TODO: Add logging system

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
    time_t time;
} TaskInfo;

// Structures

TaskInfo *tasks = NULL;
int n_tasks = 0;
char *DATA_FILENAME = ".tasks";

// Variables

void print_help() {
    printf("Usage:\n");
    printf("  flashback.exe [options]\n\n");

    printf("Available options:\n");
    printf("  --task <text>         Specifies the name or description of the task.\n");
    printf("                       Example: --task \"Check emails\"\n\n");

    printf("  --time <minutes>      Sets the time in minutes from now when the task\n");
    printf("                       should be executed.\n");
    printf("                       Example: --time 30  (runs in 30 minutes)\n\n");

    printf("  --date <MM/DD/YYYY>   Sets a specific date for the task to be executed.\n");
    printf("                       Example: --date 10/25/2025\n\n");

    printf("  --run                 Starts the scheduler in runtime mode.\n");
    printf("                       This mode listens for new tasks through the pipe\n");
    printf("                       and automatically executes them when their time arrives.\n\n");

    printf("  --help                Displays this help message.\n\n");

    printf("Examples:\n");
    printf("  1) Add a task that runs in 15 minutes:\n");
    printf("     program --task \"Turn off lights\" --time 15\n\n");

    printf("  2) Add a task for a specific date:\n");
    printf("     program --task \"Birthday reminder\" --date 10/25/2025\n\n");

    printf("  3) Run the scheduler (listening for new tasks):\n");
    printf("     program --run\n\n");

    printf("Notes:\n");
    printf("  - You must first run with --run to start the scheduler daemon.\n");
    printf("  - You must provide either --time or --date along with --task.\n");
}

int is_integer(const char *str) {
    /*
    Only checks for positive integers, with no signs.
    */
    if (str == NULL || *str == '\0')
        return 0;

    if (atoi(str) > 0) return 1;
    // Simpler and faster way

    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)str[i])) {
            return 0;
        }
    }

    return 1;
}

int is_arg_valid(int argc, char *argv[], int arg_index, char *arg_type) {
    if (arg_index >= argc || arg_index < 0) return 0;
    // Redundant check

    int arg_value_index = arg_index + 1;
    char *arg_value = argv[arg_value_index];

    if (strcmp(arg_type, "string") == 0) {
        if (strlen(arg_value) > 0 && arg_value[0] != '-') return 1;
        // Checks that the string is valid, and is not another argument
    } else if (strcmp(arg_type, "int") == 0) {
        return is_integer(arg_value);
    }
}

int parse_arguments(int argc, char *argv[], ArgsInfo *args) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--task") == 0) {
            args->task = i + 1;
            if (!is_arg_valid(argc, argv, i, "string")) return 0;
        } else if (strcmp(argv[i], "--time") == 0) {
            args->time = i + 1;
            if (!is_arg_valid(argc, argv, i, "int")) return 0;
        } else if (strcmp(argv[i], "--date") == 0) {
            args->date = i + 1;
            if (!is_arg_valid(argc, argv, i, "string")) return 0;
        } else if (strcmp(argv[i], "--run") == 0) {
            args->run = 1;
            return 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            return 0;
        } else {
            if (argv[i][0] == '-') return 0;
            // Unknown argument
        }
    }

    // Check if all the arguments were provided
    if (args->task == 0 || (args->time == 0 && args->date == 0)) {
        return 0;
    }
    
    return 1;
}

// Arguments parsing

void send_notification(char *title, char *message) {
    MessageBox(NULL, message, title, MB_OK | MB_ICONINFORMATION);
}

// Notification management

TaskInfo* allocate_task_list(TaskInfo *tasks, int n) {
    if (!tasks) {
        tasks = malloc(n * sizeof(TaskInfo));
    } else {
        tasks = realloc(tasks, n * sizeof(TaskInfo));
    }
    
    if (!tasks) printf("Error allocating memory for tasks\n");
    return tasks;
}

char* allocate_string(int n) {
    return malloc(n * sizeof(char));
}

TaskInfo *free_task_from_memory(TaskInfo *tasks, int index, int n_tasks) {
    for (int i = index; i < n_tasks - 1; i++) {
        tasks[i] = tasks[i + 1];
    }
    // Moves all tasks to the left and resizes the memory
    return realloc(tasks, (n_tasks - 1) * sizeof(TaskInfo));
}

void free_memory(void *ptr) {
    /* Frees memory and checks for errors */
    free(ptr);
    ptr = NULL;
}

// Memory management

FILE *open_file(char *filename, const char *mode) {
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

int compare_datetime_with_now(time_t time_struct) {
    time_t now = time(NULL);

    if (difftime(time_struct, now) <= 0) return 1;
    return 0;
}

int convert_str_to_time(char *time_str) {
    /*
    Convert string in minutes.
    */
    return atoi(time_str);
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

time_t convert_date_to_time(Date date) {
    struct tm timeinfo = {0};

    timeinfo.tm_year = date.year - 1900; // Year since 1900
    timeinfo.tm_mon = date.month - 1;    // Month from 0 to 11
    timeinfo.tm_mday = date.day;         // Day of the month
    timeinfo.tm_hour = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;

    return mktime(&timeinfo);
}

time_t get_final_time(time_t date, int minutes) {
    time_t final_time = date + minutes * 60;
    if (final_time <= 0) return 0;

    time_t now = time(NULL);
    return now + final_time;
}

// Date management

void save_tasks(char *data_filename, const char *mode, TaskInfo *tasks, int n_tasks) {
    if (tasks == NULL) {
        printf("No tasks to save, removing file\n");
        int status = remove(data_filename);
        if (status != 0) {
            printf("Error removing file %s\n", data_filename);
        }
        return;
    }

    FILE* data_file = open_file(data_filename, mode);
    fwrite(tasks, sizeof(char), sizeof(TaskInfo), data_file);
    fclose(data_file);
    printf("Tasks saved\n");
}

TaskInfo get_task(char *data_filename, char *task, char *date, char* time) {
    TaskInfo task_info;

    int minutes = convert_str_to_time(time);
    Date date_struct = convert_str_to_date(date);
    time_t date_time_struct = convert_date_to_time(date_struct);
    time_t final_time = get_final_time(date_time_struct, minutes);

    strcpy(task_info.task, task);
    task_info.time = final_time;

    const char *mode = (access(data_filename, F_OK) == 0) ? "ab" : "wb";
    
    return task_info;
}

TaskInfo *append_task(TaskInfo *tasks, int *n_tasks, TaskInfo task) {
    tasks = allocate_task_list(tasks, *n_tasks);

    strcpy(tasks[*n_tasks - 1].task, task.task);
    tasks[*n_tasks - 1].time = task.time;

    printf("Task '%s' appended\n", task.task);

    save_tasks(DATA_FILENAME, "ab", tasks, *n_tasks);

    return tasks;
}

TaskInfo *check_tasks_status(char *data_filename, TaskInfo *tasks, int *n_tasks) {
    if (*n_tasks == 0 || tasks == NULL) {
        printf("No tasks to check\n");
        return tasks;
    }

    printf("Ready to check\n");

    int i = 0;
    while (i < *n_tasks) {
        int status = compare_datetime_with_now(tasks[i].time);
        // status == 0 => task after now
        // status == 1 => task before now or now

        printf("Checking task %s - status: %d\n", tasks[i].task, status);

        if (status) {
            send_notification("Flashback", tasks[i].task);
            printf("Removing task %s\n", tasks[i].task);

            printf("Tasks pointer %p\n", tasks);
            tasks = free_task_from_memory(tasks, i, *n_tasks);
            (*n_tasks)--;
            printf("Tasks pointer %p\n", tasks);

            printf("Saving tasks - %d tasks\n", *n_tasks);
            save_tasks(data_filename, "wb", tasks, *n_tasks);

            continue;
        }

        i++;
    }

    return tasks;
}

void on_new_task(const char *message) {
    TaskInfo *t = (TaskInfo *) message;
    printf("Appending task '%s'\n", t->task);

    n_tasks++;

    tasks = append_task(tasks, &n_tasks, *t);
}

// Task management

int main(int argc, char *argv[]) {
    ArgsInfo args = {0};
    int success = parse_arguments(argc, argv, &args);

    if (!success) {
        print_help();
        return 1;
    }

    if (!args.run) {
        printf("Adding task %s\n", argv[args.task]);
        TaskInfo task = get_task(DATA_FILENAME, argv[args.task], argv[args.date], argv[args.time]);
        if (task.time <= 0) {
            print_help();
            return -1;
        }

        pipecomm_send_struct(PIPE_NAME, &task, sizeof(TaskInfo));
        return 0;
    }

    char path[256];
    if (check_pipe_server(path, PIPE_NAME)) {
        printf("Error: Pipe server already running\n");
        return -1;
    }
    // If the pipe server is already running, exit

    tasks = lift_data_file(DATA_FILENAME, &n_tasks);
    while (1) {
        tasks = check_tasks_status(DATA_FILENAME, tasks, &n_tasks);

        int status = pipecomm_start_server(PIPE_NAME, on_new_task);
        if (!status) {
            printf("Tasks %d\n", n_tasks);
        } else {
            printf("Error starting server\n");
            return -1;
        }

        sleep(1);
    }

    free_memory(tasks);

    return 0;
}