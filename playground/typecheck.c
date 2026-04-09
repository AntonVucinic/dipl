#define _POSIX_C_SOURCE 200809L
#include "typecheck.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "subst.h"

typedef struct {
  ExprTag s1;
  ExprTag s2;
} SortPair;

static int sort_pair_allowed(ExprTag s1, ExprTag s2) {
  /* (★,★) is always allowed */
  if (s1 == E_STAR && s2 == E_STAR)
    return 1;

  return (s1 == E_STAR && s2 == E_BOX) || (s1 == E_BOX && s2 == E_STAR) ||
         (s1 == E_BOX && s2 == E_BOX);
}

Env *env_extend(Env *env, const char *name, Expr *ty) {
  Env *e = malloc(sizeof(Env));
  e->name = strdup(name);
  e->type = ty;
  e->rest = env;
  return e;
}

Expr *env_lookup(Env *env, const char *name) {
  for (Env *e = env; e; e = e->rest)
    if (!strcmp(e->name, name))
      return e->type;
  return NULL;
}

int conv(const Expr *a, const Expr *b, Env *env) {
  Expr *a2 = whnf(a);
  Expr *b2 = whnf(b);

  if (a2->tag != b2->tag)
    return 0;

  switch (a2->tag) {
  case E_STAR:
  case E_BOX:
  case E_TRUE:
  case E_FALSE:
  case E_BOOL:
    return 1;

  case E_VAR:
    return strcmp(a2->name, b2->name) == 0;

  case E_APP:
    return conv(a2->fun, b2->fun, env) && conv(a2->arg, b2->arg, env);

  case E_LAM:
  case E_PI: {
    if (!conv(a2->type, b2->type, env))
      return 0;
    /* Rename b2's binder to a2's binder */
    Expr *b2_body_renamed =
        strcmp(a2->binder, b2->binder) == 0
            ? expr_clone(b2->body)
            : subst(b2->binder, e_var(a2->binder), b2->body);
    Env *env2 = env_extend(env, a2->binder, a2->type);
    int ok = conv(a2->body, b2_body_renamed, env2);
    return ok;
  }

  case E_IF:
    return conv(a2->cond, b2->cond, env) &&
           conv(a2->then_br, b2->then_br, env) &&
           conv(a2->else_br, b2->else_br, env);
  }
  return 0;
}

static void type_error(const char *msg, const Expr *e) {
  fprintf(stderr, "Type error: %s\n", msg);
  if (e) {
    fprintf(stderr, "  in: ");
    expr_println(e);
  }
  exit(1);
}

Expr *typecheck(Expr *e, Env *env) {
  switch (e->tag) {

  /* Axiom: ★ : □ */
  case E_STAR:
    return e_box();

  /* □ has no type in our two-level system — it's the top sort */
  case E_BOX:
    type_error("□ (Box) has no type — it is the top sort", e);
    return NULL;

  /* Primitives */
  case E_TRUE:
    return e_bool();
  case E_FALSE:
    return e_bool();
  case E_BOOL:
    return e_star(); /* Bool : ★ */

  /* Variable */
  case E_VAR: {
    Expr *ty = env_lookup(env, e->name);
    if (!ty) {
      fprintf(stderr, "Type error: unbound variable '%s'\n", e->name);
      exit(1);
    }
    return expr_clone(ty);
  }

  /*
   * If-then-else
   *   Γ ⊢ c : Bool
   *   Γ ⊢ t : T      Γ ⊢ f : T   (T must be a type: T:★)
   *   ─────────────────────────────────
   *   Γ ⊢ if c then t else f : T
   */
  case E_IF: {
    Expr *cty = typecheck(e->cond, env);
    if (!conv(cty, e_bool(), env))
      type_error("condition of 'if' must have type Bool", e->cond);
    Expr *tty = typecheck(e->then_br, env);
    Expr *fty = typecheck(e->else_br, env);
    if (!conv(tty, fty, env))
      type_error("branches of 'if' must have the same type", e);
    return tty;
  }

  /*
   * Pi / Π-type
   *   Γ ⊢ A : s1      Γ, x:A ⊢ B : s2     (s1,s2) ∈ rules
   *   ──────────────────────────────────────────────────────
   *   Γ ⊢ Πx:A.B : s2
   */
  case E_PI: {
    Expr *s1_expr = typecheck(e->type, env);
    Expr *s1_whnf = whnf(s1_expr);
    if (s1_whnf->tag != E_STAR && s1_whnf->tag != E_BOX)
      type_error("domain of Π must be a type or kind", e->type);

    Env *env2 = env_extend(env, e->binder, e->type);
    Expr *s2_expr = typecheck(e->body, env2);
    Expr *s2_whnf = whnf(s2_expr);
    if (s2_whnf->tag != E_STAR && s2_whnf->tag != E_BOX)
      type_error("codomain of Π must be a type or kind", e->body);

    if (!sort_pair_allowed(s1_whnf->tag, s2_whnf->tag)) {
      fprintf(stderr,
              "Type error: sort pair (%s, %s) not allowed at current"
              " lambda-cube vertex\n",
              s1_whnf->tag == E_STAR ? "★" : "□",
              s2_whnf->tag == E_STAR ? "★" : "□");
      exit(1);
    }
    return expr_clone(s2_whnf);
  }

  /*
   * Lambda / abstraction
   *   Γ ⊢ Πx:A.B : s     (checks the binder is well-kinded)
   *   Γ, x:A ⊢ e : B
   *   ─────────────────────────────
   *   Γ ⊢ λx:A.e : Πx:A.B
   */
  case E_LAM: {
    Expr *ann_sort = typecheck(e->type, env);
    Expr *ann_whnf = whnf(ann_sort);
    if (ann_whnf->tag != E_STAR && ann_whnf->tag != E_BOX)
      type_error("lambda annotation must be a type or kind", e->type);

    Env *env2 = env_extend(env, e->binder, e->type);
    Expr *body_ty = typecheck(e->body, env2);

    Expr *s2_sort = typecheck(body_ty, env2);
    Expr *s2_whnf = whnf(s2_sort);
    if (s2_whnf->tag != E_STAR && s2_whnf->tag != E_BOX)
      type_error("lambda body type must be a type or kind", e->body);

    if (!sort_pair_allowed(ann_whnf->tag, s2_whnf->tag)) {
      fprintf(stderr,
              "Type error: sort pair (%s, %s) not allowed at current"
              " lambda-cube vertex\n",
              ann_whnf->tag == E_STAR ? "★" : "□",
              s2_whnf->tag == E_STAR ? "★" : "□");
      exit(1);
    }

    return e_pi(e->binder, expr_clone(e->type), body_ty);
  }

  /*
   * Application
   *   Γ ⊢ f : Πx:A.B      Γ ⊢ a : A'     A ≡ A'
   *   ─────────────────────────────────────────────
   *   Γ ⊢ f a : B[x:=a]
   */
  case E_APP: {
    Expr *fty = typecheck(e->fun, env);
    Expr *fty_whnf = whnf(fty);

    if (fty_whnf->tag != E_PI) {
      fprintf(stderr, "Type error: applying a non-function\n");
      fprintf(stderr, "  function: ");
      expr_println(e->fun);
      fprintf(stderr, "  its type: ");
      expr_println(fty);
      exit(1);
    }

    Expr *aty = typecheck(e->arg, env);
    if (!conv(fty_whnf->type, aty, env)) {
      fprintf(stderr, "Type error: argument type mismatch\n");
      fprintf(stderr, "  expected: ");
      expr_println(fty_whnf->type);
      fprintf(stderr, "  got:      ");
      expr_println(aty);
      exit(1);
    }

    return subst(fty_whnf->binder, e->arg, fty_whnf->body);
  }
  }
  return NULL; /* unreachable */
}
