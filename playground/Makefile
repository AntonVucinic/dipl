SHELL = /bin/sh

.SUFFIXES:
.SUFFIXES: .c .o

LDFLAGS = -lreadline

all: lang

lang: term.o types.o type_check.o parse.o lex.o eval.o

clean:
	rm -f *.o lang
