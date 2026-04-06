#ifndef INCLUDE_playground_type_check_h_
#define INCLUDE_playground_type_check_h_

#include "term.h"
#include "types.h"

/* Simple association list for the type environment */
typedef struct Env {
  char *name;
  Type *type;
  struct Env *rest;
} Env;

Type *typecheck(Term *t, Env *env);

#endif // INCLUDE_playground_type_check_h_
