#ifndef EXPR_H
#define EXPR_H

/*
 * Unified expression type for a Pure Type System (PTS).
 *
 * Terms and types share one syntactic category.  We use *named*
 * variables internally (not de Bruijn) but carry enough information
 * to do capture-avoiding substitution correctly via fresh-name
 * generation.  This keeps the printer trivial.
 *
 * Sorts
 *   ★  (STAR) — the type of ordinary types, e.g.  Bool : ★
 *   □  (BOX)  — the type of ★, i.e.  ★ : □
 *
 * Pi types
 *   Πx:A. B   — dependent function type; when x ∉ FV(B) this is
 *               just the familiar  A -> B.
 *
 * Lambda cube position is controlled by the *axioms* and *rules*
 * tables in typecheck.c; the AST and substitution code never change.
 */

#include <stdbool.h>

typedef enum {
    /* Sorts */
    E_STAR,     /* ★  — Type */
    E_BOX,      /* □  — Kind */

    /* Intro / elim */
    E_VAR,      /* x            */
    E_APP,      /* e e          */
    E_LAM,      /* λx:A. e      */
    E_PI,       /* Πx:A. B      */

    /* Primitives (kept as built-ins so examples stay readable) */
    E_TRUE,
    E_FALSE,
    E_BOOL,     /* Bool : ★     */
    E_IF,       /* if e then e else e */
} ExprTag;

typedef struct Expr {
    ExprTag tag;

    /* E_VAR */
    char *name;

    /* E_LAM, E_PI  — binder */
    char       *binder;   /* bound variable name              */
    struct Expr *type;    /* binder's type annotation         */
    struct Expr *body;    /* body / return type               */

    /* E_APP */
    struct Expr *fun;
    struct Expr *arg;

    /* E_IF */
    struct Expr *cond;
    struct Expr *then_br;
    struct Expr *else_br;
} Expr;

/* ---- constructors ---- */
Expr *e_star(void);
Expr *e_box(void);
Expr *e_var(const char *name);
Expr *e_lam(const char *x, Expr *ty, Expr *body);
Expr *e_pi (const char *x, Expr *ty, Expr *body);
Expr *e_app(Expr *fun, Expr *arg);
Expr *e_true(void);
Expr *e_false(void);
Expr *e_bool(void);
Expr *e_if(Expr *cond, Expr *then_br, Expr *else_br);

/* ---- deep copy ---- */
Expr *expr_clone(const Expr *e);

/* ---- free variables ---- */
bool expr_free_in(const char *x, const Expr *e);

/* ---- printer ---- */
/*
 * Precedence levels used by the printer:
 *   0 — lowest  (Pi, Lam, If  —  extend as far right as possible)
 *   1 — application (left-associative)
 *   2 — atom
 */
void expr_print(const Expr *e, int prec);
void expr_println(const Expr *e);

#endif /* EXPR_H */
