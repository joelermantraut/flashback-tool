#include "logger.h"
#include <unistd.h>

static FILE *log_file = NULL;

// Internal function to get current time as string
static void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    strftime(buffer, size, "%m/%d/%y %H:%M:%S", local);
}

// Internal generic logger
static void log_message(LogLevel level, const char *fmt, va_list args) {
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    const char *level_str = (level == LOG_INFO) ? "INFO" : "ERROR";

    // Print to stdout
    printf("[%s] [%s] ", timestamp, level_str);
    vprintf(fmt, args);
    printf("\n");

    // If file logging is enabled
    if (log_file) {
        fprintf(log_file, "[%s] [%s] ", timestamp, level_str);
        vfprintf(log_file, fmt, args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
}

void log_init(const char *filename) {
    const char *mode = (access(filename, F_OK) == 0) ? "a" : "w";

    if (filename) {
        log_file = fopen(filename, mode);
        if (!log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", filename);
        }
    }
}

void log_close() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_INFO, fmt, args);
    va_end(args);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_ERROR, fmt, args);
    va_end(args);
}
