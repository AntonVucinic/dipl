#define _POSIX_C_SOURCE 200809L
#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "inductive.h"
#include "parse.h"
#include "subst.h"
#include "typecheck.h"

static Expr*
lookup_ind_or_var(const char* name)
{
  IndDef* ind = ind_lookup(name);
  if (ind)
    return e_ind(name);
  return e_var(name);
}

static Expr*
io_of(Expr* a)
{
  return e_app(e_io_ty(), a);
}

Expr*
io_typeof(const Expr* e)
{
  switch (e->tag) {

    case E_IO_TY:
      return e_pi("_", e_star(), e_star());

    case E_IO_PURE:
      return e_pi("A", e_star(), e_pi("_", e_var("A"), io_of(e_var("A"))));

    case E_IO_BIND:
      return e_pi("A",
                  e_star(),
                  e_pi("B",
                       e_star(),
                       e_pi("_",
                            io_of(e_var("A")),
                            e_pi("_",
                                 e_pi("_", e_var("A"), io_of(e_var("B"))),
                                 io_of(e_var("B"))))));

    case E_IO_PUTCHAR: {
      Expr* nat = lookup_ind_or_var("Nat");
      Expr* unit = lookup_ind_or_var("Unit");
      return e_pi("_", nat, io_of(unit));
    }

    case E_IO_GETCHAR: {
      return io_of(lookup_ind_or_var("Nat"));
    }

    case E_IO_EXIT: {
      Expr* nat = lookup_ind_or_var("Nat");
      Expr* unit = lookup_ind_or_var("Unit");
      return e_pi("_", nat, io_of(unit));
    }

    case E_IO_PUTSTR: {
      Expr* list_nat;
      IndDef* list = ind_lookup("List");
      if (list)
        list_nat = e_app(e_ind("List"), e_ind("Nat"));
      else
        list_nat = e_app(e_var("List"), e_var("Nat"));
      Expr* unit = lookup_ind_or_var("Unit");
      return e_pi("_", list_nat, io_of(unit));
    }

    default:
      return NULL;
  }
}

Expr*
io_bind_reduce(Expr** spine, int n)
{
  if (n != 4)
    return NULL;
  Expr* action = spine[2];
  Expr* cont = spine[3];

  Expr* act_w = whnf(action);

  if (act_w->tag != E_APP)
    return NULL;
  Expr* act_args[8];
  int na = 0;
  const Expr* cur = act_w;
  while (cur->tag == E_APP && na < 8) {
    act_args[na++] = cur->arg;
    cur = cur->fun;
  }
  if (cur->tag != E_IO_PURE || na != 2)
    return NULL;
  Expr* v = act_args[0];
  return e_app(expr_clone(cont), expr_clone(v));
}

static int
nat_to_int(const Expr* e)
{
  int n = 0;
  const Expr* cur = e;
  while (1) {
    Expr* w = whnf(cur);
    if (w->tag == E_VAR || w->tag == E_CON) {
      if (!strcmp(w->name, "zero"))
        return n;
    }
    if (w->tag == E_APP) {
      Expr* head = w->fun;
      Expr* hh = whnf(head);
      if ((hh->tag == E_VAR || hh->tag == E_CON) && !strcmp(hh->name, "succ")) {
        n++;
        cur = w->arg;
        continue;
      }
    }
    return -1;
  }
}

static Expr*
int_to_nat(int n)
{
  Expr* r = ind_lookup_con("zero", NULL) ? e_con("zero") : e_var("zero");
  for (int i = 0; i < n; i++) {
    Expr* s = ind_lookup_con("succ", NULL) ? e_con("succ") : e_var("succ");
    r = e_app(s, r);
  }
  return r;
}

static ExprTag
io_head(Expr* e, Expr** args, int* n_args, int cap)
{
  *n_args = 0;
  Expr* cur = e;
  while (cur->tag == E_APP && *n_args < cap) {
    args[(*n_args)++] = cur->arg;
    cur = cur->fun;
  }
  for (int i = 0, j = *n_args - 1; i < j; i++, j--) {
    Expr* tmp = args[i];
    args[i] = args[j];
    args[j] = tmp;
  }
  return cur->tag;
}

Expr*
io_run(Expr* action)
{
  Expr* e = whnf(action);

  Expr* args[16];
  int n = 0;
  ExprTag tag = io_head(e, args, &n, 16);

  switch (tag) {

    case E_IO_PURE: {
      if (n < 2) {
        fprintf(stderr, "IO error: pure applied to wrong number of args\n");
        return NULL;
      }
      return nf(args[1]);
    }

    case E_IO_BIND: {
      if (n < 4) {
        fprintf(stderr, "IO error: bind not fully applied\n");
        return NULL;
      }
      Expr* act = args[2];
      Expr* cont = args[3];

      Expr* result = io_run(act);
      if (!result)
        return NULL;

      Expr* next = whnf(e_app(expr_clone(cont), result));
      if (!next)
        return NULL;
      return io_run(next);
    }

    case E_IO_PUTCHAR: {
      if (n < 1) {
        fprintf(stderr, "IO error: putChar not applied\n");
        return NULL;
      }
      Expr* nat_val = nf(args[0]);
      int ch = nat_to_int(nat_val);
      if (ch < 0 || ch > 127) {
        fprintf(stderr, "IO error: putChar argument out of range\n");
        return NULL;
      }
      putchar(ch);
      fflush(stdout);
      Expr* tt = ind_lookup_con("tt", NULL) ? e_con("tt") : e_var("tt");
      return tt;
    }

    case E_IO_GETCHAR: {
      int ch = getchar();
      if (ch == EOF)
        ch = 0;
      return int_to_nat(ch);
    }

    case E_IO_EXIT: {
      if (n < 1) {
        fprintf(stderr, "IO error: exit not applied\n");
        return NULL;
      }
      Expr* nat_val = eval(args[0], 0);
      int code = nat_to_int(nat_val);
      if (code < 0)
        code = 1;
      exit(code);
      return NULL;
    }

    case E_IO_PUTSTR: {
      if (n < 1) {
        fprintf(stderr, "IO error: putStr not applied\n");
        return NULL;
      }
      Expr* xs = eval(args[0], 0);
      while (1) {
        Expr* w = whnf(xs);
        Expr* wa[8];
        int wn = 0;
        Expr* wh = w;
        while (wh->tag == E_APP && wn < 8) {
          wa[wn++] = wh->arg;
          wh = wh->fun;
        }
        for (int i = 0, j = wn - 1; i < j; i++, j--) {
          Expr* t = wa[i];
          wa[i] = wa[j];
          wa[j] = t;
        }
        const char* hname =
          (wh->tag == E_CON || wh->tag == E_VAR) ? wh->name : NULL;
        if (!hname || !strcmp(hname, "nil"))
          break;
        if (!strcmp(hname, "cons") && wn >= 3) {
          int ch = nat_to_int(eval(wa[1], 0));
          if (ch >= 0)
            putchar(ch);
          xs = wa[2];
          continue;
        }
        break;
      }
      fflush(stdout);
      return ind_lookup_con("tt", NULL) ? e_con("tt") : e_var("tt");
    }

    default:
      fprintf(
        stderr, "IO error: cannot run non-IO expression (tag %d)\n", (int)tag);
      expr_print(e, 0);
      fprintf(stderr, "\n");
      return NULL;
  }
}

Expr*
io_exec(const char* src)
{
  Expr* e = parse(src);
  if (!e)
    return NULL;
  Expr* ty = typecheck(e, NULL);
  if (!ty)
    return NULL;
  Expr* result = eval(e, 0);
  if (!result)
    return NULL;
  return io_run(result);
}
