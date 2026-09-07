#include "subst.h"

#include <stdio.h>
#include <string.h>

static char fresh_buf[128];

static const char*
fresh(const char* hint, const Expr* avoid_s, const Expr* avoid_e)
{
  int n = 0;
  while (1) {
    if (n == 0)
      snprintf(fresh_buf, sizeof(fresh_buf), "%s", hint);
    else
      snprintf(fresh_buf, sizeof(fresh_buf), "%s%d", hint, n);
    if (!expr_free_in(fresh_buf, avoid_s) && !expr_free_in(fresh_buf, avoid_e))
      return fresh_buf;
    n++;
  }
}

Expr*
subst(const char* x, const Expr* s, const Expr* e)
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

    case E_VAR:
      return strcmp(e->name, x) == 0 ? expr_clone(s) : e_var(e->name);

    case E_LAM:
    case E_PI: {
      Expr* new_ty = subst(x, s, e->type);
      if (strcmp(e->binder, x) == 0) {
        Expr* new_body = expr_clone(e->body);
        return e->tag == E_LAM ? e_lam(e->binder, new_ty, new_body)
                               : e_pi(e->binder, new_ty, new_body);
      }
      if (expr_free_in(e->binder, s)) {
        const char* z = fresh(e->binder, s, e->body);
        Expr* renamed = subst(e->binder, e_var(z), e->body);
        Expr* new_body = subst(x, s, renamed);
        return e->tag == E_LAM ? e_lam(z, new_ty, new_body)
                               : e_pi(z, new_ty, new_body);
      }
      Expr* new_body = subst(x, s, e->body);
      return e->tag == E_LAM ? e_lam(e->binder, new_ty, new_body)
                             : e_pi(e->binder, new_ty, new_body);
    }

    case E_APP:
      return e_app(subst(x, s, e->fun), subst(x, s, e->arg));

    case E_IF:
      return e_if(
        subst(x, s, e->cond), subst(x, s, e->then_br), subst(x, s, e->else_br));
  }
  return NULL;
}

static IotaHook iota_hook = NULL;
void
subst_set_iota_hook(IotaHook h)
{
  iota_hook = h;
}

static Expr*
collect_spine(const Expr* e, Expr** args, int* n, int cap)
{
  if (e->tag == E_APP && *n < cap) {
    Expr* head = collect_spine(e->fun, args, n, cap);
    args[(*n)++] = (Expr*)e->arg;
    return head;
  }
  return (Expr*)e;
}

Expr*
whnf(const Expr* e)
{
  if (!e)
    return NULL;
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
    case E_VAR:
    case E_LAM:
    case E_PI:
      return expr_clone(e);

    case E_APP: {
      Expr* fun = whnf(e->fun);
      if (fun->tag == E_LAM) {
        Expr* reduced = subst(fun->binder, e->arg, fun->body);
        return whnf(reduced);
      }
      if (iota_hook) {
        Expr* full = e_app(fun, expr_clone(e->arg));
        Expr* args[64];
        int n = 0;
        Expr* head = collect_spine(full, args, &n, 64);
        if (head->tag == E_ELIM ||
            (head->tag == E_VAR && head->name && head->name[0] != '\0')) {
          Expr* reduct = iota_hook(head->name, args, n);
          if (reduct)
            return whnf(reduct);
        }
        return full;
      }
      return e_app(fun, expr_clone(e->arg));
    }

    case E_IF: {
      Expr* cond = whnf(e->cond);
      if (cond->tag == E_TRUE)
        return whnf(e->then_br);
      if (cond->tag == E_FALSE)
        return whnf(e->else_br);
      return e_if(cond, expr_clone(e->then_br), expr_clone(e->else_br));
    }
  }
  return NULL;
}

Expr*
nf(const Expr* e)
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
    case E_VAR:
      return e_var(e->name);
    case E_LAM:
      return e_lam(e->binder, nf(e->type), nf(e->body));
    case E_PI:
      return e_pi(e->binder, nf(e->type), nf(e->body));
    case E_APP: {
      Expr* fun = nf(e->fun);
      Expr* arg = nf(e->arg);
      if (fun->tag == E_LAM) {
        Expr* r = subst(fun->binder, arg, fun->body);
        return nf(r);
      }
      return e_app(fun, arg);
    }
    case E_IF: {
      Expr* cond = nf(e->cond);
      if (cond->tag == E_TRUE)
        return nf(e->then_br);
      if (cond->tag == E_FALSE)
        return nf(e->else_br);
      return e_if(cond, nf(e->then_br), nf(e->else_br));
    }
  }
  return NULL;
}
