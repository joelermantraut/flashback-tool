#include "pipecomm.h"
#include <stdio.h>
#include <string.h>

#define PIPE_TIMEOUT 3000

static HANDLE hThread = NULL;
static HANDLE stopEvent = NULL;
static char global_pipe_name[256];
static PipeMessageCallback global_callback = NULL;

// Server thread
DWORD WINAPI pipecomm_server_thread(LPVOID lpParam) {
    char buffer[PIPECOMM_MAX_MESSAGE];
    DWORD bytesRead;

    while (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT) {
        HANDLE pipe = CreateNamedPipeA(
            global_pipe_name,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,  // <-- modo binario
            PIPE_UNLIMITED_INSTANCES,
            PIPECOMM_MAX_MESSAGE,
            PIPECOMM_MAX_MESSAGE,
            0,
            NULL
        );

        if (pipe == INVALID_HANDLE_VALUE) break;

        if (ConnectNamedPipe(pipe, NULL) ||
            GetLastError() == ERROR_PIPE_CONNECTED) {

            while (ReadFile(pipe, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
                if (global_callback) global_callback(buffer);
            }
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }

    return 0;
}

// Starts the pipe server
int pipecomm_start_server(const char *pipe_name, PipeMessageCallback callback) {
    if (hThread) return 0;

    sprintf(global_pipe_name, "\\\\.\\pipe\\%s", pipe_name);
    global_callback = callback;
    stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    hThread = CreateThread(NULL, 0, pipecomm_server_thread, NULL, 0, NULL);
    if (!hThread) {
        printf("[PipeComm] Error creating server thread\n");
        return -1;
    }

    return 0;
}

int check_pipe_server(char *path, const char *pipe_name) {
    sprintf(path, "\\\\.\\pipe\\%s", pipe_name);
    if (!WaitNamedPipeA(path, PIPE_TIMEOUT)) {
        printf("[PipeComm] Pipe not available: %s\n", path);
        return 0;
    }

    return 1;
}

// Send message as client
int pipecomm_send_struct(const char *pipe_name, const void *data, size_t data_size) {
    char path[256];
    if (!check_pipe_server(path, pipe_name)) return 0;

    HANDLE pipe = CreateFileA(
        path,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        printf("[PipeComm] Failed to open pipe: %lu\n", GetLastError());
        return -1;
    }

    DWORD written;
    BOOL success = WriteFile(pipe, data, (DWORD)data_size, &written, NULL);
    CloseHandle(pipe);

    return success ? 0 : -1;
}

// Stop server
void pipecomm_stop() {
    if (stopEvent) SetEvent(stopEvent);
    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = NULL;
    }
    if (stopEvent) {
        CloseHandle(stopEvent);
        stopEvent = NULL;
    }
}
