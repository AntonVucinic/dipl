#ifndef TYPECHECK_H
#define TYPECHECK_H

#include "expr.h"

/*
 * Pure Type System type checker.
 *
 * The lambda cube is navigated by choosing which (s1, s2) sort pairs
 * are allowed in the rule  Γ ⊢ Πx:A.B : s2  when  Γ ⊢ A : s1.
 *
 *   (★, ★)  — terms depending on terms       [all systems]
 *   (□, ★)  — terms depending on types       [λ2, λP2, λPω, λC]
 *   (★, □)  — types depending on terms       [λP, λP2, λPω, λC]
 *   (□, □)  — types depending on types       [λω, λPω, λC]
 *
 * Axiom:  ★ : □   (in every system here)
 */

typedef enum {
    CUBE_LAMBDA_ARROW = 0, /* λ→  — (★,★)              */
    CUBE_LAMBDA_P,         /* λP  — (★,★) (★,□)        */
    CUBE_LAMBDA_2,         /* λ2  — (★,★) (□,★)        */
    CUBE_LAMBDA_OMEGA,     /* λω  — (★,★) (□,□)        */
    CUBE_LAMBDA_P2,        /* λP2 — (★,★)(★,□)(□,★)   */
    CUBE_LAMBDA_P_OMEGA,   /* λPω — (★,★)(★,□)(□,□)   */
    CUBE_LAMBDA_C,         /* λC  — all four            */
} CubeVertex;

/* Global setting — change before calling typecheck() */
extern CubeVertex current_vertex;

/*
 * Type environment: association list of  (name, type).
 */
typedef struct Env {
    char        *name;
    Expr        *type;
    struct Env  *rest;
} Env;

Env  *env_extend (Env *env, const char *name, Expr *ty);
Expr *env_lookup (Env *env, const char *name);

/*
 * typecheck(e, env)
 *
 * Returns the type of e under env, or calls exit(1) with a message
 * on failure.  The returned Expr is freshly allocated.
 */
Expr *typecheck(Expr *e, Env *env);

/*
 * conv(a, b, env)
 *
 * Definitional equality: reduce both to whnf and compare structurally,
 * recursing into subterms.  Used by the type checker.
 */
int conv(const Expr *a, const Expr *b, Env *env);

#endif /* TYPECHECK_H */
