#ifndef PIPECOMM_H
#define PIPECOMM_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIPECOMM_MAX_MESSAGE 512

typedef void (*PipeMessageCallback)(const char *message);

int check_pipe_server(char *path, const char *pipe_name);

int pipecomm_start_server(const char *pipe_name, PipeMessageCallback callback);

int pipecomm_send_struct(const char *pipe_name, const void *data, size_t data_size);

void pipecomm_stop();

#ifdef __cplusplus
}
#endif

#endif // PIPECOMM_H
