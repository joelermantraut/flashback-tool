#include <stdio.h>
#include <string.h>

typedef struct {
    int task;
    int time;
    int date;
    int run;
} ArgsInfo;

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

int main(int argc, char *argv[]) {
    ArgsInfo args = {0};
    int success = parse_arguments(argc, argv, &args);

    if (!success) {
        print_help();
        return 1;
    }

    

    return 0;
}