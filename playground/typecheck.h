#ifndef TYPECHECK_H
#define TYPECHECK_H

#include "expr.h"

typedef struct Env {
  char *name;
  Expr *type;
  struct Env *rest;
} Env;

Env *env_extend(Env *env, const char *name, Expr *ty);
Expr *env_lookup(Env *env, const char *name);

Expr *typecheck(Expr *e, Env *env);

int conv(const Expr *a, const Expr *b, Env *env);

#endif /* TYPECHECK_H */
