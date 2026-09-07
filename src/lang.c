#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "eval.h"
#include "expr.h"
#include "inductive.h"
#include "io.h"
#include "parse.h"
#include "program.h"
#include "subst.h"
#include "typecheck.h"

static int
is_io_type(Expr* ty)
{
  Expr* w = whnf(ty);
  if (w->tag == E_IO_TY)
    return 1;
  if (w->tag == E_APP) {
    Expr* h = whnf(w->fun);
    return h->tag == E_IO_TY;
  }
  return 0;
}

static int
looks_like_file(const char* s)
{
  int len = (int)strlen(s);
  if (len >= 3 && !strcmp(s + len - 3, ".lc"))
    return 1;
  for (const char* p = s; *p; p++)
    if (*p == '/')
      return 1;
  return 0;
}

static void
repl_run(const char* src, int verbose)
{
  printf("Input:  %s\n", src);
  Expr* e = parse(src);
  if (!e)
    return;
  printf("Parsed: ");
  expr_println(e);

  struct Env* env = def_env();
  Expr* ty = typecheck(e, env);
  if (!ty)
    return;
  printf("Type:   ");
  expr_println(nf(ty));

  if (verbose)
    printf("Steps:\n");
  Expr* applied = def_apply(e);
  Expr* result = eval(applied, verbose);
  if (!result)
    return;
  printf("Result: ");
  expr_println(result);
  putchar('\n');
}

static void
repl_run_decl(const char* src)
{
  IndDef* ind = parse_inductive(src);
  if (!ind)
    return;
  printf("Defined: %s", ind->name);
  for (int i = 0; i < ind->n_params; i++) {
    printf(" (%s:", ind->params[i].name);
    expr_print(ind->params[i].type, 0);
    printf(")");
  }
  printf("\n");
}

static void
repl_run_effect(const char* src)
{
  char* name = NULL;
  Expr* body = parse_effect(src, &name);
  if (!body)
    return;

  struct Env* env = def_env();
  Expr* ty = typecheck(body, env);
  if (!ty) {
    free(name);
    return;
  }

  Expr* ty_nf = nf(ty);
  if (!is_io_type(ty_nf)) {
    fprintf(stderr,
            "Type error: effect '%s' must have type IO T, got: ",
            name ? name : "?");
    expr_println(ty_nf);
    free(name);
    return;
  }

  Expr* applied = def_apply(body);
  Expr* value = eval(applied, 0);
  if (!value) {
    free(name);
    return;
  }

  char* dup = malloc(strlen(src) + 1);
  memcpy(dup, src, strlen(src) + 1);
  def_register(dup, ty_nf, value, 1);

  io_run(value);
  free(name);
}

static void
load_prelude(const char* path)
{
  FILE* f = fopen(path, "r");
  if (!f)
    return;

  char buf[4096], stmt[16384];
  stmt[0] = '\0';

  while (fgets(buf, sizeof(buf), f)) {
    int len = (int)strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
      buf[--len] = '\0';
    const char* p = buf;
    while (*p == ' ' || *p == '\t')
      p++;
    if (!*p || (p[0] == '-' && p[1] == '-'))
      continue;

    if (stmt[0])
      strncat(stmt, " ", sizeof(stmt) - strlen(stmt) - 1);
    strncat(stmt, p, sizeof(stmt) - strlen(stmt) - 1);

    int slen = (int)strlen(stmt);
    int has_end = slen >= 3 && !strcmp(stmt + slen - 3, "end");
    int is_single = !is_inductive_decl(stmt);
    char last = slen > 0 ? stmt[slen - 1] : '\0';

    if (has_end || (is_single && last != '|' && last != '=')) {
      if (is_inductive_decl(stmt))
        parse_inductive(stmt);
      stmt[0] = '\0';
    }
  }
  if (stmt[0] && is_inductive_decl(stmt))
    parse_inductive(stmt);
  fclose(f);
}

static void
repl(void)
{
  int verbose = 0;
  printf("Commands: :q quit  :v toggle verbose  :? help\n\n");

  while (1) {
    char* input = readline("[λC]> ");
    if (!input)
      break;
    add_history(input);

    int len = (int)strlen(input);
    while (len > 0 && (input[len - 1] == ' ' || input[len - 1] == '\t'))
      input[--len] = '\0';
    if (!len) {
      free(input);
      continue;
    }

    if (!strcmp(input, ":q")) {
      free(input);
      break;
    }
    if (!strcmp(input, ":v")) {
      verbose = !verbose;
      printf("Verbose %s\n\n", verbose ? "ON" : "OFF");
      free(input);
      continue;
    }
    if (!strcmp(input, ":?")) {
      printf("Syntax:\n");
      printf("  \\x:T. e                      lambda\n");
      printf("  Pi x:T. e                    pi type\n");
      printf("  T -> U                       non-dependent arrow\n");
      printf("  elim e return M with | c b then e ... end\n");
      printf("  inductive T (p:K) : K := | c : T ... end\n");
      printf("  effect name := body          define/run an IO action\n");
      printf("IO: IO  pure  bind  putChar  putStr  getChar  exit\n");
      printf("Literals: 65  'A'  \"hello\"\n\n");
      free(input);
      continue;
    }

    if (is_inductive_decl(input))
      repl_run_decl(input);
    else if (is_effect_decl(input))
      repl_run_effect(input);
    else
      repl_run(input, verbose);

    free(input);
  }
}

int
main(int argc, char** argv)
{
  ind_init();
  load_prelude("prelude.lc");

  if (argc > 1 && looks_like_file(argv[1])) {
    return load_file(argv[1]);
  }

  if (argc > 1) {
    const char* src = argv[1];
    if (is_inductive_decl(src))
      repl_run_decl(src);
    else if (is_effect_decl(src))
      repl_run_effect(src);
    else
      repl_run(src, argc > 2 && !strcmp(argv[2], "-v"));
    return 0;
  }

  printf("=== λC  ===\n\n");
  repl();
  return 0;
}
