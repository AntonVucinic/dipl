#ifndef INCLUDE_playground_term_h_
#define INCLUDE_playground_term_h_

#include "types.h"

typedef enum { TM_VAR, TM_ABS, TM_APP, TM_TRUE, TM_FALSE, TM_IF } TermTag;

typedef struct Term {
  TermTag tag;
  /* TM_VAR */
  char *var;
  /* TM_ABS */
  char *param;
  Type *param_ty;
  struct Term *body;
  /* TM_APP */
  struct Term *fun;
  struct Term *arg;
  /* TM_IF */
  struct Term *cond;
  struct Term *then_br;
  struct Term *else_br;
} Term;

Term *mk_var(const char *name);
Term *mk_abs(const char *param, Type *ty, Term *body);
Term *mk_app(Term *fun, Term *arg);
Term *mk_true(void);
Term *mk_false(void);
Term *mk_if(Term *cond, Term *then_br, Term *else_br);

void print_term(Term *t);

#endif // INCLUDE_playground_term_h_
