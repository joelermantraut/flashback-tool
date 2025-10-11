#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

// Log levels
typedef enum {
    LOG_INFO,
    LOG_ERROR
} LogLevel;

// Initialize logger (optional: pass NULL to log only to stdout)
void log_init(const char *filename);

// Log functions
void log_info(const char *fmt, ...);
void log_error(const char *fmt, ...);

// Close logger
void log_close();

#endif
