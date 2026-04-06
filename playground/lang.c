/*
 * Simply Typed Lambda Calculus (STLC) Interpreter
 *
 * Syntax:
 *   Terms:
 *     x                     -- variable
 *     \x:T. t               -- lambda abstraction
 *     t t                   -- application
 *     true | false          -- boolean literals
 *     if t then t else t    -- conditional
 *
 *   Types:
 *     Bool                  -- boolean base type
 *     T -> T                -- function type (right-associative)
 *     (T)                   -- parenthesized type
 *
 * Examples:
 *   (\x:Bool. x) true
 *   (\f:Bool->Bool. f false) (\x:Bool. if x then false else true)
 *   if true then false else true
 */
// clang-format off: stdio.h before readline
#include <stdio.h>
#include <readline/history.h>
#include <readline/readline.h>
// clang-format on
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "parse.h"
#include "term.h"
#include "type_check.h"

void run(const char *src, int verbose) {
  printf("Input:  %s\n", src);

  Term *t = parse(src);
  printf("Parsed: ");
  print_term(t);
  printf("\n");

  Type *ty = typecheck(t, NULL);
  printf("Type:   ");
  print_type(ty);
  printf("\n");

  if (verbose)
    printf("Steps:\n");
  Term *result = eval(t, verbose);
  printf("Result: ");
  print_term(result);
  printf("\n");
  printf("\n");
}

void repl(void) {
  char *input;
  printf("Simply Typed Lambda Calculus Interpreter\n");
  printf("Syntax: \\x:T. t | t t | true | false | if t then t else t\n");
  printf("Types:  Bool | T -> T\n");
  printf("Type ':q' to quit, ':v' to toggle verbose step-by-step mode.\n\n");

  int verbose = 0;
  while (1) {
    if ((input = readline("> ")) == NULL)
      break;
    add_history(input);
    input[strcspn(input, "\n")] = '\0';
    if (!strcmp(input, ":q") || !strcmp(input, "quit") ||
        !strcmp(input, "exit")) {
      free(input);
      break;
    }
    if (!strcmp(input, ":v")) {
      verbose = !verbose;
      printf("Verbose mode %s\n\n", verbose ? "ON" : "OFF");
      free(input);
      continue;
    }
    if (!input[0]) {
      free(input);
      continue;
    }
    run(input, verbose);
    free(input);
  }
}

int main(int argc, char **argv) {
  if (argc > 1) {
    /* Run a single expression from command line */
    bool verbose = (argc > 2 && !strcmp(argv[2], "-v"));
    run(argv[1], verbose);
  } else {
    /* Built-in demo */
    printf("=== Simply Typed Lambda Calculus Interpreter ===\n\n");
    printf("--- Demo ---\n\n");

    run("true", 0);
    run("\\x:Bool. x", 0);
    run("(\\x:Bool. x) true", 0);
    run("(\\x:Bool. x) false", 0);
    run("if true then false else true", 0);
    run("if false then false else true", 0);

    /* not = \x:Bool. if x then false else true */
    run("(\\x:Bool. if x then false else true) true", 0);
    run("(\\x:Bool. if x then false else true) false", 0);

    /* Higher order: apply a function to false */
    run("(\\f:Bool->Bool. f false) (\\x:Bool. if x then false else true)", 1);

    /* Church-style and = \a:Bool. \b:Bool. if a then b else false */
    run("(\\a:Bool. \\b:Bool. if a then b else false) true true", 0);
    run("(\\a:Bool. \\b:Bool. if a then b else false) true false", 0);
    run("(\\a:Bool. \\b:Bool. if a then b else false) false true", 0);

    printf("--- REPL (type ':q' to quit) ---\n\n");
    repl();
  }
  return 0;
}
