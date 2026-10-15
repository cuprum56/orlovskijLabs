#include "win_utils.h"

#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* 1. Вывод PID и TID дочернего процесса */
    fprintf(stderr, "child: process pid=%lu thread tid=%lu ready\n",
            (unsigned long)GetCurrentProcessId(),
            (unsigned long)GetCurrentThreadId());
    fflush(stderr);

    for (;;) {
        /* 2. Чтение строки из stdin (pipe1); NULL — конец ввода (EOF) */
        char *line = win_readline(stdin);
        if (line == NULL)
            break;

        win_trim_eol(line);

        size_t len = strlen(line);
        if (len == 0) {
            fprintf(stderr, "child ERROR: empty line does not end with '.' or ';'\n");
            free(line);
            continue;
        }

        /* 3. Строка корректна, если оканчивается символом '.' или ';' */
        char last = line[len - 1];
        if (last == '.' || last == ';') {
            /* 4. Корректная строка записывается в stdout (файл) */
            if (fwrite(line, 1, len, stdout) != len || fputc('\n', stdout) == EOF ||
                fflush(stdout) == EOF) {
                fprintf(stderr,
                        "child ERROR: failed to write result to output file\n");
                free(line);
                return 1;
            }
        } else {
            /* 5. Некорректная строка — сообщение об ошибке в stderr (pipe2) */
            fprintf(stderr,
                    "child ERROR: line does not end with '.' or ';': '%s'\n",
                    line);
            fflush(stderr);
        }

        free(line);
    }

    /* 6. Завершение работы дочернего процесса */
    fprintf(stderr, "child: input finished, exiting\n");
    fflush(stderr);
    return 0;
}
