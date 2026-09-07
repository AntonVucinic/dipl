#define _POSIX_C_SOURCE 200809L
#include "expr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Expr*
alloc_expr(ExprTag tag)
{
  Expr* e = calloc(1, sizeof(Expr));
  if (!e) {
    perror("calloc");
    exit(1);
  }
  e->tag = tag;
  return e;
}

Expr*
e_star(void)
{
  return alloc_expr(E_STAR);
}
Expr*
e_box(void)
{
  return alloc_expr(E_BOX);
}
Expr*
e_true(void)
{
  return alloc_expr(E_TRUE);
}
Expr*
e_false(void)
{
  return alloc_expr(E_FALSE);
}
Expr*
e_bool(void)
{
  return alloc_expr(E_BOOL);
}

Expr*
e_var(const char* name)
{
  Expr* e = alloc_expr(E_VAR);
  e->name = strdup(name);
  return e;
}
Expr*
e_ind(const char* name)
{
  Expr* e = alloc_expr(E_IND);
  e->name = strdup(name);
  return e;
}
Expr*
e_con(const char* name)
{
  Expr* e = alloc_expr(E_CON);
  e->name = strdup(name);
  return e;
}
Expr*
e_elim(const char* name)
{
  Expr* e = alloc_expr(E_ELIM);
  e->name = strdup(name);
  return e;
}

Expr*
e_io_ty(void)
{
  return alloc_expr(E_IO_TY);
}
Expr*
e_io_pure(void)
{
  return alloc_expr(E_IO_PURE);
}
Expr*
e_io_bind(void)
{
  return alloc_expr(E_IO_BIND);
}
Expr*
e_io_putchar(void)
{
  return alloc_expr(E_IO_PUTCHAR);
}
Expr*
e_io_getchar(void)
{
  return alloc_expr(E_IO_GETCHAR);
}
Expr*
e_io_exit(void)
{
  return alloc_expr(E_IO_EXIT);
}
Expr*
e_io_putstr(void)
{
  return alloc_expr(E_IO_PUTSTR);
}

Expr*
e_lam(const char* x, Expr* ty, Expr* body)
{
  Expr* e = alloc_expr(E_LAM);
  e->binder = strdup(x);
  e->type = ty;
  e->body = body;
  return e;
}
Expr*
e_pi(const char* x, Expr* ty, Expr* body)
{
  Expr* e = alloc_expr(E_PI);
  e->binder = strdup(x);
  e->type = ty;
  e->body = body;
  return e;
}
Expr*
e_app(Expr* fun, Expr* arg)
{
  Expr* e = alloc_expr(E_APP);
  e->fun = fun;
  e->arg = arg;
  return e;
}
Expr*
e_if(Expr* cond, Expr* then_br, Expr* else_br)
{
  Expr* e = alloc_expr(E_IF);
  e->cond = cond;
  e->then_br = then_br;
  e->else_br = else_br;
  return e;
}

Expr*
expr_clone(const Expr* e)
{
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
    case E_IND:
      return e_ind(e->name);
    case E_CON:
      return e_con(e->name);
    case E_ELIM:
      return e_elim(e->name);
    case E_IO_TY:
      return e_io_ty();
    case E_IO_PURE:
      return e_io_pure();
    case E_IO_BIND:
      return e_io_bind();
    case E_IO_PUTCHAR:
      return e_io_putchar();
    case E_IO_GETCHAR:
      return e_io_getchar();
    case E_IO_EXIT:
      return e_io_exit();
    case E_IO_PUTSTR:
      return e_io_putstr();
    case E_LAM:
      return e_lam(e->binder, expr_clone(e->type), expr_clone(e->body));
    case E_PI:
      return e_pi(e->binder, expr_clone(e->type), expr_clone(e->body));
    case E_APP:
      return e_app(expr_clone(e->fun), expr_clone(e->arg));
    case E_IF:
      return e_if(
        expr_clone(e->cond), expr_clone(e->then_br), expr_clone(e->else_br));
  }
  return NULL;
}

bool
expr_free_in(const char* x, const Expr* e)
{
  if (!e)
    return false;
  switch (e->tag) {
    case E_STAR:
    case E_BOX:
    case E_TRUE:
    case E_FALSE:
    case E_BOOL:
    case E_IND:
    case E_CON:
    case E_ELIM:
    case E_IO_TY:
    case E_IO_PURE:
    case E_IO_BIND:
    case E_IO_PUTCHAR:
    case E_IO_GETCHAR:
    case E_IO_EXIT:
    case E_IO_PUTSTR:
      return false;
    case E_VAR:
      return strcmp(x, e->name) == 0;
    case E_LAM:
    case E_PI:
      if (expr_free_in(x, e->type))
        return true;
      if (strcmp(x, e->binder) == 0)
        return false;
      return expr_free_in(x, e->body);
    case E_APP:
      return expr_free_in(x, e->fun) || expr_free_in(x, e->arg);
    case E_IF:
      return expr_free_in(x, e->cond) || expr_free_in(x, e->then_br) ||
             expr_free_in(x, e->else_br);
  }
  return false;
}

static void
open_paren(int n)
{
  if (n)
    putchar('(');
}
static void
close_paren(int n)
{
  if (n)
    putchar(')');
}

void
expr_print(const Expr* e, int prec)
{
  if (!e) {
    printf("<null>");
    return;
  }
  switch (e->tag) {
    case E_STAR:
      printf("\xe2\x98\x85");
      return;
    case E_BOX:
      printf("\xe2\x96\xa1");
      return;
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
    case E_IND:
      printf("%s", e->name);
      return;
    case E_CON:
      printf("%s", e->name);
      return;
    case E_ELIM:
      printf("%s", e->name);
      return;
    case E_IO_TY:
      printf("IO");
      return;
    case E_IO_PURE:
      printf("pure");
      return;
    case E_IO_BIND:
      printf("bind");
      return;
    case E_IO_PUTCHAR:
      printf("putChar");
      return;
    case E_IO_GETCHAR:
      printf("getChar");
      return;
    case E_IO_EXIT:
      printf("exit");
      return;
    case E_IO_PUTSTR:
      printf("putStr");
      return;

    case E_LAM: {
      int p = prec > 0;
      open_paren(p);
      printf("\xce\xbb%s:", e->binder);
      expr_print(e->type, 0);
      printf(". ");
      expr_print(e->body, 0);
      close_paren(p);
      return;
    }
    case E_PI: {
      int p = prec > 0;
      open_paren(p);
      if (!expr_free_in(e->binder, e->body)) {
        expr_print(e->type, 1);
        printf(" \xe2\x86\x92 ");
        expr_print(e->body, 0);
      } else {
        printf("\xce\xa0%s:", e->binder);
        expr_print(e->type, 0);
        printf(". ");
        expr_print(e->body, 0);
      }
      close_paren(p);
      return;
    }
    case E_APP: {
      int p = prec > 1;
      open_paren(p);
      expr_print(e->fun, 1);
      putchar(' ');
      expr_print(e->arg, 2);
      close_paren(p);
      return;
    }
    case E_IF: {
      int p = prec > 0;
      open_paren(p);
      printf("if ");
      expr_print(e->cond, 1);
      printf(" then ");
      expr_print(e->then_br, 0);
      printf(" else ");
      expr_print(e->else_br, 0);
      close_paren(p);
      return;
    }
  }
}

void
expr_println(const Expr* e)
{
  expr_print(e, 0);
  putchar('\n');
}
