#include "type_check.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Env *env_extend(Env *env, const char *name, Type *ty) {
  Env *e = malloc(sizeof(Env));
  e->name = strdup(name);
  e->type = ty;
  e->rest = env;
  return e;
}

Type *env_lookup(Env *env, const char *name) {
  for (Env *e = env; e; e = e->rest)
    if (!strcmp(e->name, name))
      return e->type;
  return NULL;
}

Type *typecheck(Term *t, Env *env) {
  switch (t->tag) {
  case TM_TRUE:
  case TM_FALSE:
    return mk_bool();

  case TM_VAR: {
    Type *ty = env_lookup(env, t->var);
    if (!ty) {
      fprintf(stderr, "Type error: unbound variable '%s'\n", t->var);
      exit(1);
    }
    return ty;
  }

  case TM_ABS: {
    Env *env2 = env_extend(env, t->param, t->param_ty);
    Type *body_ty = typecheck(t->body, env2);
    return mk_arr(t->param_ty, body_ty);
  }

  case TM_APP: {
    Type *fun_ty = typecheck(t->fun, env);
    Type *arg_ty = typecheck(t->arg, env);
    if (fun_ty->tag != TY_ARR) {
      fprintf(stderr, "Type error: applying non-function\n");
      fprintf(stderr, "  Function has type: ");
      print_type(fun_ty);
      fprintf(stderr, "\n");
      exit(1);
    }
    if (!type_eq(fun_ty->param, arg_ty)) {
      fprintf(stderr, "Type error: argument type mismatch\n");
      fprintf(stderr, "  Expected: ");
      print_type(fun_ty->param);
      fprintf(stderr, "\n");
      fprintf(stderr, "  Got:      ");
      print_type(arg_ty);
      fprintf(stderr, "\n");
      exit(1);
    }
    return fun_ty->ret;
  }

  case TM_IF: {
    Type *cond_ty = typecheck(t->cond, env);
    if (!type_eq(cond_ty, mk_bool())) {
      fprintf(stderr, "Type error: condition must be Bool\n");
      exit(1);
    }
    Type *then_ty = typecheck(t->then_br, env);
    Type *else_ty = typecheck(t->else_br, env);
    if (!type_eq(then_ty, else_ty)) {
      fprintf(stderr, "Type error: branches of 'if' must have the same type\n");
      fprintf(stderr, "  then: ");
      print_type(then_ty);
      fprintf(stderr, "\n");
      fprintf(stderr, "  else: ");
      print_type(else_ty);
      fprintf(stderr, "\n");
      exit(1);
    }
    return then_ty;
  }
  }
  return NULL; /* unreachable */
}
