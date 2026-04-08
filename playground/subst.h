#ifndef SUBST_H
#define SUBST_H

#include "expr.h"

/*
 * subst(x, s, e)  — capture-avoiding substitution  e[x := s]
 *
 * When the bound variable of a Lam/Pi would capture a free variable
 * of s, we alpha-rename the binder to a fresh name before descending.
 */
Expr *subst(const char *x, const Expr *s, const Expr *e);

/*
 * whnf(e)  — reduce e to weak-head normal form
 *
 * Reduces the outermost redex first (beta for App-Lam, conditional
 * for If-True/False), then does NOT reduce under binders.
 * Used by the type checker's conversion test.
 */
Expr *whnf(const Expr *e);

/*
 * nf(e)  — reduce e to full normal form (for printing results)
 */
Expr *nf(const Expr *e);

#endif /* SUBST_H */
