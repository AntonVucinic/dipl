#ifndef IO_H
#define IO_H

#include "expr.h"

Expr*
io_typeof(const Expr* e);
Expr*
io_bind_reduce(Expr** spine, int n);
Expr*
io_run(Expr* action);
Expr*
io_exec(const char* src);

#endif /* IO_H */
