#ifndef WIN_UTILS_H
#define WIN_UTILS_H

#include <windows.h>
#include <stdio.h>
#include <stddef.h>

#if defined(_MSC_VER)
#  define WIN_NORETURN __declspec(noreturn)
#else
#  define WIN_NORETURN __attribute__((noreturn))
#endif

/* 1. Аварийное завершение с текстом ошибки Win32 */
WIN_NORETURN void win_fail(const char *context);

/* 2. Выделение памяти с проверкой */
void *win_alloc(size_t size);

/* 3. Перевыделение памяти с проверкой */
void *win_realloc(void *ptr, size_t size);

/* 4. Чтение строки произвольной длины */
char *win_readline(FILE *stream);

/* 5. Удаление символов конца строки */
void win_trim_eol(char *line);

#endif
