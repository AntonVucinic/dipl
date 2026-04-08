#ifndef PARSE_H
#define PARSE_H

#include "expr.h"

/*
 * parse(src)  — parse a complete expression from a string.
 *
 * Grammar (informal):
 *
 *   expr  ::= '\' x ':' expr '.' expr      -- λ-abstraction
 *           | 'Pi' x ':' expr '.' expr     -- Π-type
 *           | 'if' expr 'then' expr 'else' expr
 *           | app ('->' expr)*             -- right-assoc non-dep Pi sugar
 *           | app
 *
 *   app   ::= atom+                        -- left-associative
 *
 *   atom  ::= x  |  '*'  |  '□'  |  'Bool'  |  'true'  |  'false'
 *           | '(' expr ')'
 *
 * '->' is sugar for Π_ :A. B  when the variable is not needed.
 */
Expr *parse(const char *src);

#endif /* PARSE_H */
