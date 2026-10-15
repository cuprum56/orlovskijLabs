#include "win_utils.h"

#include <stdlib.h>
#include <string.h>

/* 1. Аварийное завершение с текстом ошибки Win32 */
void win_fail(const char *context)
{
    DWORD err = GetLastError();
    char *msg = NULL;

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&msg, 0, NULL);

    fprintf(stderr, "[fatal] %s failed (Win32 error %lu): %s\n", context,
            (unsigned long)err, msg ? msg : "unknown error");

    if (msg != NULL)
        LocalFree(msg);

    ExitProcess(1);
}

/* 2. Выделение памяти с проверкой */
void *win_alloc(size_t size)
{
    void *p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "[fatal] out of memory (requested %zu bytes)\n", size);
        ExitProcess(1);
    }
    return p;
}

/* 3. Перевыделение памяти с проверкой */
void *win_realloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);
    if (p == NULL) {
        fprintf(stderr, "[fatal] out of memory (requested %zu bytes)\n", size);
        ExitProcess(1);
    }
    return p;
}

/* 4. Чтение строки произвольной длины */
char *win_readline(FILE *stream)
{
    size_t cap = 128;
    size_t len = 0;
    char *buf = win_alloc(cap);
    int c;

    while ((c = fgetc(stream)) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            buf = win_realloc(buf, cap);
        }
        buf[len++] = (char)c;
        if (c == '\n')
            break;
    }

    if (c == EOF && len == 0) {
        free(buf);
        return NULL;
    }

    buf[len] = '\0';
    return buf;
}

/* 5. Удаление символов конца строки */
void win_trim_eol(char *line)
{
    size_t n = strlen(line);
    while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
        line[--n] = '\0';
}
