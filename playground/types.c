#include "types.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static Type *ty_bool_singleton = NULL;

Type *mk_bool(void) {
  if (!ty_bool_singleton) {
    ty_bool_singleton = malloc(sizeof(Type));
    ty_bool_singleton->tag = TY_BOOL;
    ty_bool_singleton->param = ty_bool_singleton->ret = NULL;
  }
  return ty_bool_singleton;
}

Type *mk_arr(Type *param, Type *ret) {
  Type *t = malloc(sizeof(Type));
  t->tag = TY_ARR;
  t->param = param;
  t->ret = ret;
  return t;
}

bool type_eq(Type *a, Type *b) {
  if (a->tag != b->tag)
    return 0;
  if (a->tag == TY_BOOL)
    return 1;
  return type_eq(a->param, b->param) && type_eq(a->ret, b->ret);
}

void print_type(Type *t) {
  if (t->tag == TY_BOOL) {
    printf("Bool");
  } else {
    /* Parenthesize left side if it's also an arrow (right-associative) */
    if (t->param->tag == TY_ARR) {
      printf("(");
      print_type(t->param);
      printf(")");
    } else {
      print_type(t->param);
    }
    printf(" -> ");
    print_type(t->ret);
  }
}
