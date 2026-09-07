#include "eval.h"

#include <stdio.h>
#include <stdlib.h>

#include "inductive.h"
#include "io.h"
#include "subst.h"

static int is_value(const Expr *e) {
  switch (e->tag) {
  case E_STAR: case E_BOX:
  case E_TRUE: case E_FALSE: case E_BOOL:
  case E_LAM:  case E_PI:
  case E_IND:  case E_CON:  case E_ELIM:
  case E_IO_TY: case E_IO_PURE: case E_IO_BIND:
  case E_IO_PUTCHAR: case E_IO_GETCHAR: case E_IO_EXIT: case E_IO_PUTSTR:
    return 1;
  default: return 0;
  }
}

static Expr *spine_head(const Expr *e, Expr **args, int *n) {
  if (e->tag == E_APP) {
    Expr *h = spine_head(e->fun, args, n);
    args[(*n)++] = (Expr *)e->arg;
    return h;
  }
  return (Expr *)e;
}

static Expr *step(const Expr *e) {
  switch (e->tag) {
  case E_STAR: case E_BOX:
  case E_TRUE: case E_FALSE: case E_BOOL:
  case E_IND:  case E_CON:  case E_ELIM:
  case E_IO_TY: case E_IO_PURE: case E_IO_BIND:
  case E_IO_PUTCHAR: case E_IO_GETCHAR: case E_IO_EXIT: case E_IO_PUTSTR:
  case E_VAR:
    return NULL;

  case E_LAM: {
    Expr *ty2 = step(e->type);
    if (ty2) return e_lam(e->binder, ty2, expr_clone(e->body));
    Expr *b2  = step(e->body);
    if (b2)  return e_lam(e->binder, expr_clone(e->type), b2);
    return NULL;
  }
  case E_PI: {
    Expr *ty2 = step(e->type);
    if (ty2) return e_pi(e->binder, ty2, expr_clone(e->body));
    Expr *b2  = step(e->body);
    if (b2)  return e_pi(e->binder, expr_clone(e->type), b2);
    return NULL;
  }

  case E_APP: {
    Expr *args[64]; int n = 0;
    Expr *head = spine_head(e, args, &n);

    if (head->tag == E_ELIM ||
        (head->tag == E_VAR && ind_lookup_elim(head->name))) {
      IndDef *ind = ind_lookup_elim(head->name);
      if (ind) {
        int nu = ind->n_params - ind->n_indices;
        int expected = nu + 1 + ind->n_con + ind->n_indices + 1;
        if (n == expected) {
          args[n-1] = whnf(args[n-1]);
          Expr *reduct = ind_iota(ind, args, n);
          if (reduct) return reduct;
        }
      }
    }

    if (head->tag == E_IO_BIND && n == 4) {
      Expr *reduct = io_bind_reduce(args, n);
      if (reduct) return reduct;
    }

    if (!is_value(e->fun)) {
      Expr *f2 = step(e->fun);
      if (f2) return e_app(f2, expr_clone(e->arg));
    }
    if (!is_value(e->arg)) {
      Expr *a2 = step(e->arg);
      if (a2) return e_app(expr_clone(e->fun), a2);
    }
    if (e->fun->tag == E_LAM)
      return subst(e->fun->binder, e->arg, e->fun->body);
    return NULL;
  }

  case E_IF: {
    if (e->cond->tag == E_TRUE)  return expr_clone(e->then_br);
    if (e->cond->tag == E_FALSE) return expr_clone(e->else_br);
    Expr *c2 = step(e->cond);
    if (c2) return e_if(c2, expr_clone(e->then_br), expr_clone(e->else_br));
    return NULL;
  }
  }
  return NULL;
}

Expr *eval(Expr *e, int verbose) {
  int steps = 0;
  while (1) {
    Expr *e2 = step(e);
    if (!e2) return e;
    steps++;
    if (verbose) { printf("  \xe2\x86\x92 "); expr_println(e2); }
    e = e2;
    if (steps > 10000000) {
      fprintf(stderr, "Evaluation error: too many steps\n");
      return NULL;
    }
  }
}
