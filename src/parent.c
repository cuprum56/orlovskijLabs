#include "win_utils.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    HANDLE pipe2_read;
} child_error_reader_t;

static DWORD WINAPI child_error_reader(LPVOID param)
{
    child_error_reader_t *ctx = (child_error_reader_t *)param;
    char buffer[4096];
    DWORD read;

    printf("[parent/reader] thread tid=%lu started\n",
           (unsigned long)GetCurrentThreadId());
    fflush(stdout);

    for (;;) {
        if (!ReadFile(ctx->pipe2_read, buffer, sizeof(buffer), &read, NULL)) {
            DWORD err = GetLastError();
            if (err == ERROR_BROKEN_PIPE)
                break;
            win_fail("ReadFile(pipe2)");
        }
        if (read == 0)
            break;

        fwrite(buffer, 1, read, stdout);
        fflush(stdout);
    }
    return 0;
}

static void resolve_child_path(const char *override, char *out, size_t out_size)
{
    if (override != NULL) {
        strncpy(out, override, out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    char module[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, module, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        win_fail("GetModuleFileNameA");

    char *slash = strrchr(module, '\\');
    if (slash != NULL)
        slash[1] = '\0';
    else
        module[0] = '\0';

    int n = snprintf(out, out_size, "%schild.exe", module);
    if (n < 0 || n >= (int)out_size) {
        fprintf(stderr, "[fatal] path to child.exe is too long\n");
        ExitProcess(1);
    }
}

int main(int argc, char *argv[])
{
    char child_path[MAX_PATH];
    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE pipe1_read = NULL, pipe1_write = NULL;
    HANDLE pipe2_read = NULL, pipe2_write = NULL;
    HANDLE reader = NULL, process = NULL;
    PROCESS_INFORMATION pi;
    DWORD child_exit_code = 0u;
    int result = 0;

    ZeroMemory(&pi, sizeof(pi));

    printf("[parent] process pid=%lu main-thread tid=%lu\n",
           (unsigned long)GetCurrentProcessId(),
           (unsigned long)GetCurrentThreadId());

    /* 1. Имя файла результата */
    printf("Enter result file name: ");
    fflush(stdout);

    char *first = win_readline(stdin);
    if (first == NULL) {
        fprintf(stderr, "[fatal] no file name was provided\n");
        return 1;
    }
    win_trim_eol(first);
    if (first[0] == '\0') {
        fprintf(stderr, "[fatal] empty file name\n");
        free(first);
        return 1;
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    file = CreateFileA(first, GENERIC_WRITE, FILE_SHARE_READ, &sa,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        win_fail("CreateFileA(result file)");
    printf("[parent] results will be written to '%s'\n", first);
    free(first);

    /* 2. Создание двух безымянных каналов */
    if (!CreatePipe(&pipe1_read, &pipe1_write, &sa, 0))
        win_fail("CreatePipe(pipe1)");
    if (!CreatePipe(&pipe2_read, &pipe2_write, &sa, 0))
        win_fail("CreatePipe(pipe2)");

    if (!SetHandleInformation(pipe1_write, HANDLE_FLAG_INHERIT, 0))
        win_fail("SetHandleInformation(pipe1_write)");
    if (!SetHandleInformation(pipe2_read, HANDLE_FLAG_INHERIT, 0))
        win_fail("SetHandleInformation(pipe2_read)");

    /* 3. Запуск дочернего процесса с перенаправлением стандартных потоков */
    resolve_child_path(argc > 1 ? argv[1] : NULL, child_path, sizeof(child_path));

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = pipe1_read;
    si.hStdOutput = file;
    si.hStdError = pipe2_write;

    char command_line[MAX_PATH + 4];
    int cmd_len = snprintf(command_line, sizeof(command_line), "\"%s\"", child_path);
    if (cmd_len < 0 || cmd_len >= (int)sizeof(command_line)) {
        fprintf(stderr, "[fatal] command line is too long\n");
        result = 1;
        goto cleanup;
    }

    printf("[parent] launching '%s'\n", child_path);
    if (!CreateProcessA(child_path, command_line, NULL, NULL,
                        TRUE, 0, NULL, NULL, &si, &pi))
        win_fail("CreateProcessA(child)");

    process = pi.hProcess;

    printf("[parent] child started: pid=%lu main-thread tid=%lu\n",
           (unsigned long)pi.dwProcessId, (unsigned long)pi.dwThreadId);

    CloseHandle(pipe1_read);
    pipe1_read = NULL;
    CloseHandle(pipe2_write);
    pipe2_write = NULL;
    CloseHandle(file);
    file = INVALID_HANDLE_VALUE;

    /* 4. Поток-читатель ошибок из pipe2 (защита от взаимоблокировки) */
    child_error_reader_t reader_ctx;
    reader_ctx.pipe2_read = pipe2_read;

    reader = CreateThread(NULL, 0, child_error_reader, &reader_ctx, 0, NULL);
    if (reader == NULL)
        win_fail("CreateThread");

    /* 5. Пересылка строк пользователя в pipe1 (stdin потомка) */
    printf("[parent] Enter strings. Each must end with '.' or ';'.\n");
    printf("[parent] Press Ctrl+Z then Enter to finish.\n");
    fflush(stdout);

    char *line;
    while ((line = win_readline(stdin)) != NULL) {
        size_t length = strlen(line);
        DWORD written = 0;

        if (!WriteFile(pipe1_write, line, (DWORD)length, &written, NULL)) {
            DWORD err = GetLastError();
            free(line);
            if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA)
                break;
            win_fail("WriteFile(pipe1)");
        }
        free(line);
    }

    CloseHandle(pipe1_write);
    pipe1_write = NULL;

    /* 6. Ожидание завершения и освобождение ресурсов */
    if (WaitForSingleObject(reader, INFINITE) != WAIT_OBJECT_0)
        win_fail("WaitForSingleObject(reader)");

    if (WaitForSingleObject(process, INFINITE) != WAIT_OBJECT_0)
        win_fail("WaitForSingleObject(child)");

    if (!GetExitCodeProcess(process, &child_exit_code))
        win_fail("GetExitCodeProcess");

    printf("[parent] child finished with exit code %lu\n",
           (unsigned long)child_exit_code);

cleanup:
    if (reader != NULL)
        CloseHandle(reader);
    if (process != NULL)
        CloseHandle(process);
    if (pi.hThread != NULL)
        CloseHandle(pi.hThread);
    if (pipe1_read != NULL)
        CloseHandle(pipe1_read);
    if (pipe1_write != NULL)
        CloseHandle(pipe1_write);
    if (pipe2_read != NULL)
        CloseHandle(pipe2_read);
    if (pipe2_write != NULL)
        CloseHandle(pipe2_write);
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);

    return result;
}
