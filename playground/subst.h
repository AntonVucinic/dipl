#ifndef SUBST_H
#define SUBST_H

#include "expr.h"

Expr *subst(const char *x, const Expr *s, const Expr *e);

Expr *whnf(const Expr *e);

Expr *nf(const Expr *e);

#endif /* SUBST_H */
