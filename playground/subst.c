#include "subst.h"

#include <stdio.h>
#include <string.h>

static char fresh_buf[128];

static const char *fresh(const char *hint, const Expr *avoid_s,
                         const Expr *avoid_e) {
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

Expr *subst(const char *x, const Expr *s, const Expr *e) {
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
    if (strcmp(e->name, x) == 0)
      return expr_clone(s);
    return e_var(e->name);

  case E_LAM:
  case E_PI: {
    /* Always substitute in the type annotation */
    Expr *new_ty = subst(x, s, e->type);

    /* Binder shadows x — body is unchanged, but type still subst'd */
    if (strcmp(e->binder, x) == 0) {
      Expr *new_body = expr_clone(e->body);
      return (e->tag == E_LAM) ? e_lam(e->binder, new_ty, new_body)
                               : e_pi(e->binder, new_ty, new_body);
    }

    /* Binder would capture a free var of s — alpha-rename */
    if (expr_free_in(e->binder, s)) {
      const char *z = fresh(e->binder, s, e->body);
      Expr *renamed_body = subst(e->binder, e_var(z), e->body);
      Expr *new_body = subst(x, s, renamed_body);
      return (e->tag == E_LAM) ? e_lam(z, new_ty, new_body)
                               : e_pi(z, new_ty, new_body);
    }

    /* Safe to substitute directly */
    Expr *new_body = subst(x, s, e->body);
    return (e->tag == E_LAM) ? e_lam(e->binder, new_ty, new_body)
                             : e_pi(e->binder, new_ty, new_body);
  }

  case E_APP:
    return e_app(subst(x, s, e->fun), subst(x, s, e->arg));

  case E_IF:
    return e_if(subst(x, s, e->cond), subst(x, s, e->then_br),
                subst(x, s, e->else_br));
  }
  return NULL; /* unreachable */
}

Expr *whnf(const Expr *e) {
  if (!e)
    return NULL;
  switch (e->tag) {
  /* Already canonical */
  case E_STAR:
  case E_BOX:
  case E_TRUE:
  case E_FALSE:
  case E_BOOL:
  case E_VAR:
  case E_LAM:
  case E_PI:
    return expr_clone(e);

  case E_APP: {
    Expr *fun = whnf(e->fun);
    if (fun->tag == E_LAM) {
      /* β-reduction */
      Expr *reduced = subst(fun->binder, e->arg, fun->body);
      Expr *result = whnf(reduced);
      return result;
    }
    /* fun is not a lambda — return App with reduced fun */
    return e_app(fun, expr_clone(e->arg));
  }

  case E_IF: {
    Expr *cond = whnf(e->cond);
    if (cond->tag == E_TRUE)
      return whnf(e->then_br);
    if (cond->tag == E_FALSE)
      return whnf(e->else_br);
    return e_if(cond, expr_clone(e->then_br), expr_clone(e->else_br));
  }
  }
  return NULL; /* unreachable */
}

Expr *nf(const Expr *e) {
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
    return e_lam(e->binder, nf(e->type), nf(e->body));
  case E_PI:
    return e_pi(e->binder, nf(e->type), nf(e->body));

  case E_APP: {
    Expr *fun = nf(e->fun);
    Expr *arg = nf(e->arg);
    if (fun->tag == E_LAM) {
      Expr *reduced = subst(fun->binder, arg, fun->body);
      return nf(reduced);
    }
    return e_app(fun, arg);
  }

  case E_IF: {
    Expr *cond = nf(e->cond);
    if (cond->tag == E_TRUE)
      return nf(e->then_br);
    if (cond->tag == E_FALSE)
      return nf(e->else_br);
    return e_if(cond, nf(e->then_br), nf(e->else_br));
  }
  }
  return NULL;
}
