SHELL  = /bin/sh

CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -g -Og
LDFLAGS = -lreadline

.SUFFIXES:
.SUFFIXES: .c .o

OBJS = expr.o subst.o inductive.o io.o lex.o parse.o typecheck.o eval.o program.o

all: lang

lang: $(OBJS)
	$(CC) $(CFLAGS) -o lang $(OBJS) lang.c $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o lang

.PHONY: all clean
