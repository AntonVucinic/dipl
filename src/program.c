#define _POSIX_C_SOURCE 200809L
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "inductive.h"
#include "io.h"
#include "parse.h"
#include "subst.h"
#include "typecheck.h"

static Def* def_table = NULL;

void
def_register(char* name, Expr* type, Expr* value, int is_effect)
{
  Def** p = &def_table;
  while (*p) {
    if (!strcmp((*p)->name, name)) {
      Def* old = *p;
      *p = old->next;
      free(old->name);
      free(old);
      break;
    }
    p = &(*p)->next;
  }
  Def* d = calloc(1, sizeof(Def));
  d->name = name;
  d->type = type;
  d->value = value;
  d->is_effect = is_effect;
  d->next = def_table;
  def_table = d;
}

Def*
def_lookup(const char* name)
{
  for (Def* d = def_table; d; d = d->next)
    if (!strcmp(d->name, name))
      return d;
  return NULL;
}

struct Env*
def_env(void)
{
  struct Env* env = NULL;
  int n = 0;
  for (Def* d = def_table; d; d = d->next)
    n++;
  Def** arr = malloc(n * sizeof(Def*));
  int i = 0;
  for (Def* d = def_table; d; d = d->next)
    arr[i++] = d;
  for (int j = n - 1; j >= 0; j--)
    env = env_extend(env, arr[j]->name, arr[j]->type);
  free(arr);
  return env;
}

Expr*
def_apply(const Expr* e)
{
  Expr* result = expr_clone(e);
  for (Def* d = def_table; d; d = d->next) {
    if (expr_free_in(d->name, result)) {
      Expr* next = subst(d->name, d->value, result);
      result = next;
    }
  }
  return result;
}

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
process_stmt(const char* src, int is_file_mode)
{
  if (!src || !src[0])
    return 0;

  while (*src == ' ' || *src == '\t')
    src++;
  if (!src[0] || (src[0] == '-' && src[1] == '-'))
    return 0;

  if (is_inductive_decl(src)) {
    IndDef* ind = parse_inductive(src);
    if (!ind)
      return 1;
    if (!is_file_mode)
      printf("Defined: %s\n", ind->name);
    return 0;
  }

  if (is_effect_decl(src)) {
    char* name = NULL;
    Expr* body = parse_effect(src, &name);
    if (!body)
      return 1;

    struct Env* env = def_env();
    Expr* ty = typecheck(body, env);
    if (!ty) {
      fprintf(stderr, "In effect '%s': typecheck failed\n", name ? name : "?");
      free(name);
      return 1;
    }

    Expr* ty_nf = nf(ty);
    if (!is_io_type(ty_nf)) {
      fprintf(stderr,
              "Type error: effect '%s' must have type IO T, got: ",
              name ? name : "?");
      expr_println(ty_nf);
      free(name);
      return 1;
    }

    Expr* applied = def_apply(body);
    Expr* value = eval(applied, 0);
    if (!value) {
      free(name);
      return 1;
    }

    if (is_file_mode) {
      def_register(name, ty_nf, value, 1);
    } else {
      def_register(strdup(name), ty_nf, value, 1);
      io_run(value);
      free(name);
    }
    return 0;
  }

  if (!is_file_mode) {
    Expr* e = parse(src);
    if (!e)
      return 1;
    struct Env* env = def_env();
    Expr* ty = typecheck(e, env);
    if (!ty)
      return 1;
    printf("Type:   ");
    expr_println(nf(ty));
    Expr* applied = def_apply(e);
    Expr* result = eval(applied, 0);
    if (!result)
      return 1;
    printf("Result: ");
    expr_println(result);
    putchar('\n');
  }
  return 0;
}

int
load_file(const char* path)
{
  FILE* f = fopen(path, "r");
  if (!f) {
    fprintf(stderr, "Error: cannot open '%s'\n", path);
    return 1;
  }

  char buf[4096];
  char stmt[65536];
  stmt[0] = '\0';
  int errors = 0;
  int in_inductive = 0;
  int paren_depth = 0;
  int lineno = 0;

  while (fgets(buf, sizeof(buf), f)) {
    lineno++;
    int len = (int)strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
      buf[--len] = '\0';

    const char* p = buf;
    while (*p == ' ' || *p == '\t')
      p++;
    if (!in_inductive && (!*p || (p[0] == '-' && p[1] == '-')))
      continue;

    int cont = (len > 0 && buf[len - 1] == '\\');
    if (cont)
      buf[--len] = '\0';

    if (stmt[0]) {
      size_t sl = strlen(stmt), bl = strlen(p);
      if (sl + bl + 2 < sizeof(stmt)) {
        stmt[sl] = ' ';
        stmt[sl + 1] = '\0';
        strncat(stmt, p, sizeof(stmt) - sl - 2);
      }
    } else {
      strncpy(stmt, p, sizeof(stmt) - 1);
      stmt[sizeof(stmt) - 1] = '\0';
    }

    if (is_inductive_decl(stmt))
      in_inductive = 1;

    for (const char* cp = p; *cp; cp++) {
      if (*cp == '(')
        paren_depth++;
      else if (*cp == ')')
        paren_depth--;
    }

    int slen = (int)strlen(stmt);
    int has_end = slen >= 3 && !strcmp(stmt + slen - 3, "end");

    int flush = 0;
    if (cont) {
      flush = 0;
    } else if (in_inductive) {
      flush = has_end;
    } else if (paren_depth > 0) {
      flush = 0;
    } else {
      int slen2 = (int)strlen(stmt);
      int ends_assign =
        slen2 >= 2 && stmt[slen2 - 1] == '=' && stmt[slen2 - 2] == ':';
      flush = !ends_assign;
    }

    if (flush) {
      if (process_stmt(stmt, 1) != 0) {
        fprintf(stderr, "  (in file '%s', near line %d)\n", path, lineno);
        errors++;
      }
      stmt[0] = '\0';
      in_inductive = 0;
      paren_depth = 0;
    }
  }

  if (stmt[0]) {
    if (process_stmt(stmt, 1) != 0) {
      fprintf(stderr, "  (in file '%s', near end)\n", path);
      errors++;
    }
  }
  fclose(f);

  if (errors > 0) {
    fprintf(stderr, "%d error(s) in '%s'\n", errors, path);
    return 1;
  }

  Def* main_def = def_lookup("main");
  if (!main_def) {
    fprintf(stderr, "Error: no 'main' effect defined in '%s'\n", path);
    return 1;
  }
  if (!is_io_type(main_def->type)) {
    fprintf(stderr, "Error: 'main' must have type IO T, got: ");
    expr_println(main_def->type);
    return 1;
  }

  io_run(main_def->value);
  return 0;
}
