#ifndef SUBST_H
#define SUBST_H

#include "expr.h"

Expr*
subst(const char* x, const Expr* s, const Expr* e);
Expr*
whnf(const Expr* e);
Expr*
nf(const Expr* e);

typedef Expr* (*IotaHook)(const char* elim_name, Expr** args, int n);
void
subst_set_iota_hook(IotaHook hook);

#endif /* SUBST_H */
