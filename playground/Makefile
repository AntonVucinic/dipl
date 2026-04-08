SHELL  = /bin/sh
CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

.SUFFIXES:
.SUFFIXES: .c .o

OBJS = expr.o subst.o lex.o parse.o typecheck.o eval.o lang.o

all: lang

lang: $(OBJS)
	$(CC) $(CFLAGS) -o lang $(OBJS) $(LDFLAGS)

# Try to use readline; fall back gracefully if headers are absent
READLINE_AVAILABLE := $(shell echo '\#include <readline/readline.h>' | \
	$(CC) -x c - -c -o /dev/null 2>/dev/null && echo yes || echo no)

ifeq ($(READLINE_AVAILABLE),yes)
CFLAGS  += -DHAVE_READLINE
LDFLAGS += -lreadline
endif

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o lang

.PHONY: all clean
