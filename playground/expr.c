#define _POSIX_C_SOURCE 200809L
#include "expr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Expr *alloc_expr(ExprTag tag) {
  Expr *e = calloc(1, sizeof(Expr));
  if (!e) {
    perror("calloc");
    exit(1);
  }
  e->tag = tag;
  return e;
}

Expr *e_star(void) { return alloc_expr(E_STAR); }
Expr *e_box(void) { return alloc_expr(E_BOX); }
Expr *e_true(void) { return alloc_expr(E_TRUE); }
Expr *e_false(void) { return alloc_expr(E_FALSE); }
Expr *e_bool(void) { return alloc_expr(E_BOOL); }

Expr *e_var(const char *name) {
  Expr *e = alloc_expr(E_VAR);
  e->name = strdup(name);
  return e;
}

Expr *e_lam(const char *x, Expr *ty, Expr *body) {
  Expr *e = alloc_expr(E_LAM);
  e->binder = strdup(x);
  e->type = ty;
  e->body = body;
  return e;
}

Expr *e_pi(const char *x, Expr *ty, Expr *body) {
  Expr *e = alloc_expr(E_PI);
  e->binder = strdup(x);
  e->type = ty;
  e->body = body;
  return e;
}

Expr *e_app(Expr *fun, Expr *arg) {
  Expr *e = alloc_expr(E_APP);
  e->fun = fun;
  e->arg = arg;
  return e;
}

Expr *e_if(Expr *cond, Expr *then_br, Expr *else_br) {
  Expr *e = alloc_expr(E_IF);
  e->cond = cond;
  e->then_br = then_br;
  e->else_br = else_br;
  return e;
}

Expr *expr_clone(const Expr *e) {
  if (!e)
    return NULL;
  switch (e->tag) {
  case E_STAR:
    return e_star();
  case E_BOX:
    return e_box();
  case E_TRUE:
    return e_true();
  case E_FALSE:
    return e_false();
  case E_BOOL:
    return e_bool();
  case E_VAR:
    return e_var(e->name);
  case E_LAM:
    return e_lam(e->binder, expr_clone(e->type), expr_clone(e->body));
  case E_PI:
    return e_pi(e->binder, expr_clone(e->type), expr_clone(e->body));
  case E_APP:
    return e_app(expr_clone(e->fun), expr_clone(e->arg));
  case E_IF:
    return e_if(expr_clone(e->cond), expr_clone(e->then_br),
                expr_clone(e->else_br));
  }
  return NULL; /* unreachable */
}

bool expr_free_in(const char *x, const Expr *e) {
  if (!e)
    return false;
  switch (e->tag) {
  case E_STAR:
  case E_BOX:
  case E_TRUE:
  case E_FALSE:
  case E_BOOL:
    return false;
  case E_VAR:
    return strcmp(x, e->name) == 0;
  case E_LAM:
  case E_PI:
    if (expr_free_in(x, e->type))
      return true;
    if (strcmp(x, e->binder) == 0)
      return false; /* shadowed */
    return expr_free_in(x, e->body);
  case E_APP:
    return expr_free_in(x, e->fun) || expr_free_in(x, e->arg);
  case E_IF:
    return expr_free_in(x, e->cond) || expr_free_in(x, e->then_br) ||
           expr_free_in(x, e->else_br);
  }
  return false;
}

static void print_parens_open(int need) {
  if (need)
    putchar('(');
}
static void print_parens_close(int need) {
  if (need)
    putchar(')');
}

void expr_print(const Expr *e, int prec) {
  if (!e) {
    printf("<null>");
    return;
  }
  switch (e->tag) {
  case E_STAR:
    printf("\xe2\x98\x85");
    return; /* ★ */
  case E_BOX:
    printf("\xe2\x96\xa1");
    return; /* □ */
  case E_TRUE:
    printf("true");
    return;
  case E_FALSE:
    printf("false");
    return;
  case E_BOOL:
    printf("Bool");
    return;
  case E_VAR:
    printf("%s", e->name);
    return;

  case E_LAM: {
    int need = (prec > 0);
    print_parens_open(need);
    printf("\xce\xbb%s:", e->binder); /* λ */
    expr_print(e->type, 0);
    printf(". ");
    expr_print(e->body, 0);
    print_parens_close(need);
    return;
  }

  case E_PI: {
    int need = (prec > 0);
    print_parens_open(need);
    if (!expr_free_in(e->binder, e->body)) {
      expr_print(e->type, 1);
      printf(" \xe2\x86\x92 "); /* → */
      expr_print(e->body, 0);
    } else {
      printf("\xce\xa0%s:", e->binder); /* Π */
      expr_print(e->type, 0);
      printf(". ");
      expr_print(e->body, 0);
    }
    print_parens_close(need);
    return;
  }

  case E_APP: {
    int need = (prec > 1);
    print_parens_open(need);
    expr_print(e->fun, 1);
    putchar(' ');
    expr_print(e->arg, 2);
    print_parens_close(need);
    return;
  }

  case E_IF: {
    int need = (prec > 0);
    print_parens_open(need);
    printf("if ");
    expr_print(e->cond, 1);
    printf(" then ");
    expr_print(e->then_br, 0);
    printf(" else ");
    expr_print(e->else_br, 0);
    print_parens_close(need);
    return;
  }
  }
}

void expr_println(const Expr *e) {
  expr_print(e, 0);
  putchar('\n');
}
