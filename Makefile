CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -Werror -pedantic -std=c99 -Isrc/include
LDFLAGS ?=
RM      ?= rm -f

.PHONY: all clean

all: parent.exe child.exe

win_utils.o: src/win_utils.c src/include/win_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

parent.o: src/parent.c src/include/win_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

child.o: src/child.c src/include/win_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

parent.exe: win_utils.o parent.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

child.exe: win_utils.o child.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	$(RM) *.o *.exe
