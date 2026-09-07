#ifndef PARSE_H
#define PARSE_H

#include "expr.h"
#include "inductive.h"

Expr*
parse(const char* src);

IndDef*
parse_inductive(const char* src);

int
is_inductive_decl(const char* src);

Expr*
parse_effect(const char* src, char** name_out);

int
is_effect_decl(const char* src);

#endif /* PARSE_H */
