#ifndef EXPR_H
#define EXPR_H

#include <stdbool.h>

typedef enum
{
  E_STAR,
  E_BOX,
  E_VAR,
  E_APP,
  E_LAM,
  E_PI,
  E_TRUE,
  E_FALSE,
  E_BOOL,
  E_IF,
  E_IND,
  E_CON,
  E_ELIM,

  E_IO_TY,
  E_IO_PURE,
  E_IO_BIND,
  E_IO_PUTCHAR,
  E_IO_GETCHAR,
  E_IO_EXIT,
  E_IO_PUTSTR,
} ExprTag;

typedef struct Expr
{
  ExprTag tag;
  char* name;
  char* binder;
  struct Expr* type;
  struct Expr* body;
  struct Expr* fun;
  struct Expr* arg;
  struct Expr* cond;
  struct Expr* then_br;
  struct Expr* else_br;
} Expr;

Expr*
e_star(void);
Expr*
e_box(void);
Expr*
e_var(const char* name);
Expr*
e_lam(const char* x, Expr* ty, Expr* body);
Expr*
e_pi(const char* x, Expr* ty, Expr* body);
Expr*
e_app(Expr* fun, Expr* arg);
Expr*
e_true(void);
Expr*
e_false(void);
Expr*
e_bool(void);
Expr*
e_if(Expr* cond, Expr* then_br, Expr* else_br);
Expr*
e_ind(const char* name);
Expr*
e_con(const char* name);
Expr*
e_elim(const char* name);

Expr*
e_io_ty(void);
Expr*
e_io_pure(void);
Expr*
e_io_bind(void);
Expr*
e_io_putchar(void);
Expr*
e_io_getchar(void);
Expr*
e_io_exit(void);
Expr*
e_io_putstr(void);

Expr*
expr_clone(const Expr* e);
bool
expr_free_in(const char* x, const Expr* e);
void
expr_print(const Expr* e, int prec);
void
expr_println(const Expr* e);

#endif /* EXPR_H */
