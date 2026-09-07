#ifndef INDUCTIVE_H
#define INDUCTIVE_H

#include "expr.h"

typedef struct
{
  char* name;
  Expr* type;
} Param;

typedef struct
{
  char* name;
  Expr* type;
  int arity;
  int* rec_args;
  int n_rec;
} ConDef;

typedef struct IndDef
{
  char* name;
  int n_params;
  Param* params;
  int* is_index;
  int n_indices;
  Expr* arity;
  int n_con;
  ConDef* cons;
  Expr* elim_type;
  struct IndDef* next;
} IndDef;

void
ind_init(void);

IndDef*
ind_register(char* name,
             int n_params,
             Param* params,
             Expr* arity,
             int n_con,
             ConDef* cons);

IndDef*
ind_lookup(const char* name);
IndDef*
ind_lookup_con(const char* con_name, int* idx);
IndDef*
ind_lookup_elim(const char* elim_name);

Expr*
ind_elim_type(IndDef* ind);

Expr*
ind_iota(IndDef* ind, Expr** spine, int spine_len);

#endif /* INDUCTIVE_H */
