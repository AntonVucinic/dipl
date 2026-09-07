#ifndef PROGRAM_H
#define PROGRAM_H

#include "expr.h"

typedef struct Def
{
  char* name;
  Expr* type;
  Expr* value;
  int is_effect;
  struct Def* next;
} Def;

void
def_register(char* name, Expr* type, Expr* value, int is_effect);

Def*
def_lookup(const char* name);

struct Env*
def_env(void);

Expr*
def_apply(const Expr* e);

int
load_file(const char* path);

#endif /* PROGRAM_H */
