// clang-format off
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
// clang-format on

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "expr.h"
#include "parse.h"
#include "typecheck.h"

static void run(const char *src, int verbose) {
  printf("Input:  %s\n", src);

  Expr *e = parse(src);
  printf("Parsed: ");
  expr_println(e);

  Expr *ty = typecheck(e, NULL);
  printf("Type:   ");
  expr_println(ty);

  if (verbose)
    printf("Steps:\n");
  Expr *result = eval(e, verbose);
  printf("Result: ");
  expr_println(result);
  putchar('\n');
}

static void repl(void) {
  int verbose = 0;

  while (1) {
    char prompt[32];
    snprintf(prompt, sizeof(prompt), "[λC]> ");

    char *input = readline(prompt);
    if (!input)
      break;
    add_history(input);

    /* Strip trailing whitespace */
    int len = (int)strlen(input);
    while (len > 0 && (input[len - 1] == ' ' || input[len - 1] == '\t'))
      input[--len] = '\0';

    if (!len) {
      free(input);
      continue;
    }

    run(input, verbose);
    free(input);
  }
}

static void demo(void) {
  run("\\A:*. \\x:A. x", 0);
  run("Pi A:*. A -> A", 0);
  run("(\\A:*. A -> A) Bool", 0);
  run("Pi A:*. Pi B:(A -> *). *", 0);
}

int main(int argc, char **argv) {
  if (argc > 1) {
    /* :vertex flag */
    int i = 1;
    bool verbose = false;
    if (i < argc && !strcmp(argv[i], "-v")) {
      verbose = true;
      i++;
    }
    if (i < argc) {
      run(argv[i], verbose);
      return 0;
    }
  }

  demo();
  printf("--- REPL ---\n\n");
  repl();
  return 0;
}
