#ifndef INCLUDE_playground_types_h_
#define INCLUDE_playground_types_h_

#include "stdbool.h"

typedef enum { TY_BOOL, TY_ARR } TypeTag;

typedef struct Type {
  TypeTag tag;
  struct Type *param; /* TY_ARR only */
  struct Type *ret;   /* TY_ARR only */
} Type;

Type *mk_bool(void);
Type *mk_arr(Type *param, Type *ret);

bool type_eq(Type *a, Type *b);

void print_type(Type *t);

#endif // INCLUDE_playground_types_h_
