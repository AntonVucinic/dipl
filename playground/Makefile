SHELL  = /bin/sh

CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g -Og
LDFLAGS = -lreadline

.SUFFIXES:
.SUFFIXES: .c .o

OBJS = expr.o subst.o lex.o parse.o typecheck.o eval.o

all: lang

lang: $(OBJS)

clean:
	rm -f *.o lang

.PHONY: all clean
