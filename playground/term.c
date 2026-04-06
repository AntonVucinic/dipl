#include "term.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Term *mk_var(const char *name) {
  Term *t = calloc(1, sizeof(Term));
  t->tag = TM_VAR;
  t->var = strdup(name);
  return t;
}

Term *mk_abs(const char *param, Type *ty, Term *body) {
  Term *t = calloc(1, sizeof(Term));
  t->tag = TM_ABS;
  t->param = strdup(param);
  t->param_ty = ty;
  t->body = body;
  return t;
}

Term *mk_app(Term *fun, Term *arg) {
  Term *t = calloc(1, sizeof(Term));
  t->tag = TM_APP;
  t->fun = fun;
  t->arg = arg;
  return t;
}

Term *mk_true(void) {
  Term *t = calloc(1, sizeof(Term));
  t->tag = TM_TRUE;
  return t;
}
Term *mk_false(void) {
  Term *t = calloc(1, sizeof(Term));
  t->tag = TM_FALSE;
  return t;
}

Term *mk_if(Term *cond, Term *then_br, Term *else_br) {
  Term *t = calloc(1, sizeof(Term));
  t->tag = TM_IF;
  t->cond = cond;
  t->then_br = then_br;
  t->else_br = else_br;
  return t;
}

void print_term(Term *t) {
  switch (t->tag) {
  case TM_VAR:
    printf("%s", t->var);
    break;
  case TM_TRUE:
    printf("true");
    break;
  case TM_FALSE:
    printf("false");
    break;
  case TM_ABS:
    printf("(\\%s:", t->param);
    print_type(t->param_ty);
    printf(". ");
    print_term(t->body);
    printf(")");
    break;
  case TM_APP:
    printf("(");
    print_term(t->fun);
    printf(" ");
    print_term(t->arg);
    printf(")");
    break;
  case TM_IF:
    printf("(if ");
    print_term(t->cond);
    printf(" then ");
    print_term(t->then_br);
    printf(" else ");
    print_term(t->else_br);
    printf(")");
    break;
  }
}
