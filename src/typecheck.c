#define _POSIX_C_SOURCE 200809L
#include "typecheck.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inductive.h"
#include "io.h"
#include "subst.h"

static int
sort_pair_allowed(ExprTag s1, ExprTag s2)
{
  if (s1 == E_STAR && s2 == E_STAR)
    return 1;
  return (s1 == E_STAR && s2 == E_BOX) || (s1 == E_BOX && s2 == E_STAR) ||
         (s1 == E_BOX && s2 == E_BOX);
}

Env*
env_extend(Env* env, const char* name, Expr* ty)
{
  Env* e = malloc(sizeof(Env));
  e->name = strdup(name);
  e->type = ty;
  e->rest = env;
  return e;
}

Expr*
env_lookup(Env* env, const char* name)
{
  for (Env* e = env; e; e = e->rest)
    if (!strcmp(e->name, name))
      return e->type;
  return NULL;
}

static const char*
expr_name(const Expr* e)
{
  switch (e->tag) {
    case E_VAR:
    case E_IND:
    case E_CON:
    case E_ELIM:
      return e->name;
    default:
      return NULL;
  }
}

int
conv(const Expr* a, const Expr* b, Env* env)
{
  Expr *a2 = whnf(a), *b2 = whnf(b);

  const char *na = expr_name(a2), *nb = expr_name(b2);
  if (na && nb)
    return strcmp(na, nb) == 0;
  if (na || nb)
    return 0;

  if (a2->tag != b2->tag)
    return 0;
  switch (a2->tag) {
    case E_STAR:
    case E_BOX:
    case E_TRUE:
    case E_FALSE:
    case E_BOOL:
    case E_IO_TY:
    case E_IO_PURE:
    case E_IO_BIND:
    case E_IO_PUTCHAR:
    case E_IO_GETCHAR:
    case E_IO_EXIT:
    case E_IO_PUTSTR:
      return 1;
    case E_APP:
      return conv(a2->fun, b2->fun, env) && conv(a2->arg, b2->arg, env);
    case E_LAM:
    case E_PI: {
      if (!conv(a2->type, b2->type, env))
        return 0;
      Expr* b2r = strcmp(a2->binder, b2->binder) == 0
                    ? expr_clone(b2->body)
                    : subst(b2->binder, e_var(a2->binder), b2->body);
      Env* env2 = env_extend(env, a2->binder, a2->type);
      return conv(a2->body, b2r, env2);
    }
    case E_IF:
      return conv(a2->cond, b2->cond, env) &&
             conv(a2->then_br, b2->then_br, env) &&
             conv(a2->else_br, b2->else_br, env);
    default:
      return 0;
  }
}

static void
type_error(const char* msg, const Expr* e)
{
  fprintf(stderr, "Type error: %s\n", msg);
  if (e) {
    fprintf(stderr, "  in: ");
    expr_println(e);
  }
}

static Expr*
fill_annotations(const Expr* e, const Expr* expected)
{
  if (!e || !expected)
    return expr_clone(e);
  if (e->tag != E_LAM)
    return expr_clone(e);
  if (e->type->tag != E_VAR || strcmp(e->type->name, "_") != 0)
    return expr_clone(e);
  Expr* exp_w = whnf(expected);
  if (exp_w->tag != E_PI)
    return expr_clone(e);
  Expr* filled_body = fill_annotations(e->body, exp_w->body);
  return e_lam(e->binder, expr_clone(exp_w->type), filled_body);
}

Expr*
typecheck(Expr* e, Env* env)
{
  switch (e->tag) {

    case E_STAR:
      return e_box();

    case E_BOX:
      type_error("□ has no type — it is the top sort", e);
      return NULL;

    case E_TRUE:
      return e_bool();
    case E_FALSE:
      return e_bool();
    case E_BOOL:
      return e_star();

    case E_IO_TY:
    case E_IO_PURE:
    case E_IO_BIND:
    case E_IO_PUTCHAR:
    case E_IO_GETCHAR:
    case E_IO_EXIT:
    case E_IO_PUTSTR: {
      Expr* t = io_typeof(e);
      if (!t) {
        fprintf(stderr, "Type error: unknown IO primitive\n");
        return NULL;
      }
      return t;
    }

    case E_VAR: {
      Expr* ty = env_lookup(env, e->name);
      if (ty)
        return expr_clone(ty);
      int ci;
      IndDef* ind = ind_lookup_con(e->name, &ci);
      if (ind)
        return expr_clone(ind->cons[ci].type);
      ind = ind_lookup_elim(e->name);
      if (ind)
        return expr_clone(ind->elim_type);
      ind = ind_lookup(e->name);
      if (ind) {
        Expr* kind = expr_clone(ind->arity);
        for (int i = ind->n_params - 1; i >= 0; i--)
          kind =
            e_pi(ind->params[i].name, expr_clone(ind->params[i].type), kind);
        return kind;
      }
      fprintf(stderr, "Type error: unbound variable '%s'\n", e->name);
      return NULL;
    }

    case E_IND: {
      IndDef* ind = ind_lookup(e->name);
      if (!ind) {
        fprintf(stderr, "Type error: unknown inductive type '%s'\n", e->name);
        return NULL;
      }
      Expr* kind = expr_clone(ind->arity);
      for (int i = ind->n_params - 1; i >= 0; i--)
        kind = e_pi(ind->params[i].name, expr_clone(ind->params[i].type), kind);
      return kind;
    }

    case E_CON: {
      int ci;
      IndDef* ind = ind_lookup_con(e->name, &ci);
      if (!ind) {
        fprintf(stderr, "Type error: unknown constructor '%s'\n", e->name);
        return NULL;
      }
      return expr_clone(ind->cons[ci].type);
    }

    case E_ELIM: {
      IndDef* ind = ind_lookup_elim(e->name);
      if (!ind) {
        fprintf(stderr, "Type error: unknown eliminator '%s'\n", e->name);
        return NULL;
      }
      return expr_clone(ind->elim_type);
    }

    case E_IF: {
      Expr* cty = typecheck(e->cond, env);
      if (!cty)
        return NULL;
      if (!conv(cty, e_bool(), env)) {
        type_error("condition of 'if' must have type Bool", e->cond);
        return NULL;
      }
      Expr* tty = typecheck(e->then_br, env);
      if (!tty)
        return NULL;
      Expr* fty = typecheck(e->else_br, env);
      if (!fty)
        return NULL;
      if (!conv(tty, fty, env)) {
        type_error("branches of 'if' must have the same type", e);
        return NULL;
      }
      return tty;
    }

    case E_PI: {
      Expr* s1e = typecheck(e->type, env);
      if (!s1e)
        return NULL;
      Expr* s1 = whnf(s1e);
      if (s1->tag != E_STAR && s1->tag != E_BOX) {
        type_error("domain of Π must be a type or kind", e->type);
        return NULL;
      }
      Env* env2 = env_extend(env, e->binder, e->type);
      Expr* s2e = typecheck(e->body, env2);
      if (!s2e)
        return NULL;
      Expr* s2 = whnf(s2e);
      if (s2->tag != E_STAR && s2->tag != E_BOX) {
        type_error("codomain of Π must be a type or kind", e->body);
        return NULL;
      }
      if (!sort_pair_allowed(s1->tag, s2->tag)) {
        fprintf(stderr,
                "Type error: sort pair (%s, %s) not allowed\n",
                s1->tag == E_STAR ? "★" : "□",
                s2->tag == E_STAR ? "★" : "□");
        return NULL;
      }
      return expr_clone(s2);
    }

    case E_LAM: {
      Expr* as = typecheck(e->type, env);
      if (!as)
        return NULL;
      Expr* aw = whnf(as);
      if (aw->tag != E_STAR && aw->tag != E_BOX) {
        type_error("lambda annotation must be a type or kind", e->type);
        return NULL;
      }
      Env* env2 = env_extend(env, e->binder, e->type);
      Expr* bty = typecheck(e->body, env2);
      if (!bty)
        return NULL;
      Expr* bs = typecheck(bty, env2);
      if (!bs)
        return NULL;
      Expr* bw = whnf(bs);
      if (bw->tag != E_STAR && bw->tag != E_BOX) {
        type_error("lambda body type must be a type or kind", e->body);
        return NULL;
      }
      if (!sort_pair_allowed(aw->tag, bw->tag)) {
        fprintf(stderr,
                "Type error: sort pair (%s, %s) not allowed\n",
                aw->tag == E_STAR ? "★" : "□",
                bw->tag == E_STAR ? "★" : "□");
        return NULL;
      }
      return e_pi(e->binder, expr_clone(e->type), bty);
    }

    case E_APP: {
      Expr* fty = typecheck(e->fun, env);
      if (!fty)
        return NULL;
      Expr* fw = whnf(fty);
      if (fw->tag != E_PI) {
        fprintf(stderr, "Type error: applying a non-function\n");
        fprintf(stderr, "  function: ");
        expr_println(e->fun);
        fprintf(stderr, "  its type: ");
        expr_println(fty);
        return NULL;
      }

      Expr* arg = fill_annotations(e->arg, fw->type);

      Expr* aty = typecheck(arg, env);
      if (!aty)
        return NULL;
      if (!conv(fw->type, aty, env)) {
        fprintf(stderr, "Type error: argument type mismatch\n");
        fprintf(stderr, "  expected: ");
        expr_println(fw->type);
        fprintf(stderr, "  got:      ");
        expr_println(aty);
        return NULL;
      }
      return subst(fw->binder, arg, fw->body);
    }
  }
  return NULL;
}
