#ifndef EVAL_H
#define EVAL_H

#include "expr.h"

/*
 * eval(e, verbose)
 *
 * Reduce e to full normal form via iterated small steps.
 * If verbose != 0, print each intermediate step.
 */
Expr *eval(Expr *e, int verbose);

#endif /* EVAL_H */
