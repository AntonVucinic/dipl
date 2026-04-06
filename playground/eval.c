#include "eval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Substitution: [x := s] t */
Term *subst(const char *x, Term *s, Term *t) {
  switch (t->tag) {
  case TM_TRUE:
    return mk_true();
  case TM_FALSE:
    return mk_false();

  case TM_VAR:
    return strcmp(t->var, x) == 0 ? s : mk_var(t->var);

  case TM_ABS:
    if (!strcmp(t->param, x))
      return mk_abs(t->param, t->param_ty, t->body); /* shadowed */
    /* TODO: alpha-rename if x appears free in t->body and param is free in s.
       For simplicity we assume no variable capture in these examples. */
    return mk_abs(t->param, t->param_ty, subst(x, s, t->body));

  case TM_APP:
    return mk_app(subst(x, s, t->fun), subst(x, s, t->arg));

  case TM_IF:
    return mk_if(subst(x, s, t->cond), subst(x, s, t->then_br),
                 subst(x, s, t->else_br));
  }
  return NULL;
}

int is_value(Term *t) {
  return t->tag == TM_TRUE || t->tag == TM_FALSE || t->tag == TM_ABS;
}

/* Returns NULL if no step possible (i.e., already a value or stuck) */
Term *step(Term *t) {
  switch (t->tag) {
  case TM_TRUE:
  case TM_FALSE:
  case TM_ABS:
    return NULL; /* values, no step */

  case TM_VAR:
    return NULL; /* stuck (shouldn't happen after type check) */

  case TM_APP: {
    /* E-App1: step the function */
    if (!is_value(t->fun)) {
      Term *fun2 = step(t->fun);
      return fun2 ? mk_app(fun2, t->arg) : NULL;
    }
    /* E-App2: function is a value, step the argument */
    if (!is_value(t->arg)) {
      Term *arg2 = step(t->arg);
      return arg2 ? mk_app(t->fun, arg2) : NULL;
    }
    /* E-AppAbs: beta reduction */
    if (t->fun->tag == TM_ABS) {
      return subst(t->fun->param, t->arg, t->fun->body);
    }
    return NULL; /* stuck */
  }

  case TM_IF: {
    /* E-IfTrue / E-IfFalse */
    if (t->cond->tag == TM_TRUE)
      return t->then_br;
    if (t->cond->tag == TM_FALSE)
      return t->else_br;
    /* E-If: step condition */
    Term *cond2 = step(t->cond);
    return cond2 ? mk_if(cond2, t->then_br, t->else_br) : NULL;
  }
  }
  return NULL;
}

/* Multi-step evaluation (big step via iterated small steps) */
Term *eval(Term *t, int verbose) {
  int steps = 0;
  while (!is_value(t)) {
    Term *t2 = step(t);
    if (!t2) {
      fprintf(stderr, "Evaluation error: stuck term\n");
      exit(1);
    }
    steps++;
    if (verbose) {
      printf("  step %d: ", steps);
      print_term(t2);
      printf("\n");
    }
    t = t2;
    if (steps > 10000) {
      fprintf(stderr, "Evaluation error: too many steps (infinite loop?)\n");
      exit(1);
    }
  }
  return t;
}
