/*
 * Simply Typed Lambda Calculus → Lambda Cube interpreter
 * Driver: demo + readline REPL
 */

/* ---- readline (optional) ---- */
#ifdef HAVE_READLINE
#  include <readline/readline.h>
#  include <readline/history.h>
#else
/* Minimal fgets-based fallback so we compile without libreadline headers */
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
static char *readline(const char *prompt) {
    printf("%s", prompt);
    fflush(stdout);
    char *buf = malloc(4096);
    if (!buf) return NULL;
    if (!fgets(buf, 4096, stdin)) { free(buf); return NULL; }
    buf[strcspn(buf, "\n")] = '\0';
    return buf;
}
static void add_history(const char *s) { (void)s; }
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "expr.h"
#include "parse.h"
#include "typecheck.h"

/* ------------------------------------------------------------------ */
/* Vertex name table                                                    */
/* ------------------------------------------------------------------ */

static const struct { CubeVertex v; const char *name; } VERTICES[] = {
    { CUBE_LAMBDA_ARROW,   "λ→"   },
    { CUBE_LAMBDA_P,       "λP"   },
    { CUBE_LAMBDA_2,       "λ2"   },
    { CUBE_LAMBDA_OMEGA,   "λω"   },
    { CUBE_LAMBDA_P2,      "λP2"  },
    { CUBE_LAMBDA_P_OMEGA, "λPω"  },
    { CUBE_LAMBDA_C,       "λC"   },
};
#define N_VERTICES (int)(sizeof(VERTICES)/sizeof(VERTICES[0]))

static const char *vertex_name(CubeVertex v) {
    for (int i = 0; i < N_VERTICES; i++)
        if (VERTICES[i].v == v) return VERTICES[i].name;
    return "?";
}

/* ------------------------------------------------------------------ */
/* Run one expression                                                   */
/* ------------------------------------------------------------------ */

static void run(const char *src, int verbose) {
    printf("Input:  %s\n", src);

    Expr *e = parse(src);
    printf("Parsed: "); expr_println(e);

    Expr *ty = typecheck(e, NULL);
    printf("Type:   "); expr_println(ty);

    if (verbose) printf("Steps:\n");
    Expr *result = eval(e, verbose);
    printf("Result: "); expr_println(result);
    putchar('\n');
}

/* ------------------------------------------------------------------ */
/* REPL                                                                 */
/* ------------------------------------------------------------------ */

static void print_help(void) {
    printf("Commands:\n");
    printf("  :q               quit\n");
    printf("  :v               toggle verbose (step-by-step) mode\n");
    printf("  :vertex <name>   switch lambda-cube vertex\n");
    printf("                   names: λ→  λP  λ2  λω  λP2  λPω  λC\n");
    printf("  :vertex          show current vertex\n");
    printf("  :help            this message\n\n");
    printf("Syntax:\n");
    printf("  \\x:T. e          lambda abstraction (also λ)\n");
    printf("  Pi x:T. e        pi type (also Π)\n");
    printf("  T -> U           non-dependent function type\n");
    printf("  *  or  ★         the sort Type\n");
    printf("  BOX or □         the sort Kind\n");
    printf("  Bool  true  false  if/then/else\n\n");
}

static void repl(void) {
    print_help();
    int verbose = 0;

    while (1) {
        /* Show current vertex in prompt */
        char prompt[32];
        snprintf(prompt, sizeof(prompt), "[%s]> ", vertex_name(current_vertex));

        char *input = readline(prompt);
        if (!input) break;
        add_history(input);

        /* Strip trailing whitespace */
        int len = (int)strlen(input);
        while (len > 0 && (input[len-1] == ' ' || input[len-1] == '\t'))
            input[--len] = '\0';

        if (!len) { free(input); continue; }

        /* ---- Meta-commands ---- */
        if (!strcmp(input, ":q") || !strcmp(input, "quit")) {
            free(input); break;
        }
        if (!strcmp(input, ":v")) {
            verbose = !verbose;
            printf("Verbose mode %s\n\n", verbose ? "ON" : "OFF");
            free(input); continue;
        }
        if (!strcmp(input, ":help") || !strcmp(input, ":h")) {
            print_help(); free(input); continue;
        }
        if (!strcmp(input, ":vertex")) {
            printf("Current vertex: %s\n\n", vertex_name(current_vertex));
            free(input); continue;
        }
        if (!strncmp(input, ":vertex ", 8)) {
            const char *vname = input + 8;
            int found = 0;
            for (int i = 0; i < N_VERTICES; i++) {
                if (!strcmp(vname, VERTICES[i].name)) {
                    current_vertex = VERTICES[i].v;
                    printf("Switched to vertex %s\n\n", VERTICES[i].name);
                    found = 1; break;
                }
            }
            if (!found) {
                printf("Unknown vertex '%s'. Try: λ→ λP λ2 λω λP2 λPω λC\n\n",
                       vname);
            }
            free(input); continue;
        }

        run(input, verbose);
        free(input);
    }
}

/* ------------------------------------------------------------------ */
/* Built-in demo                                                        */
/* ------------------------------------------------------------------ */

static void demo(void) {
    printf("=== Lambda Cube Interpreter ===\n\n");

    /* ---- λ→ demo ---- */
    printf("--- Vertex λ→ (simply typed) ---\n\n");
    current_vertex = CUBE_LAMBDA_ARROW;
    run("true", 0);
    run("\\x:Bool. x", 0);
    run("(\\x:Bool. x) true", 0);
    run("if true then false else true", 0);
    run("(\\f:Bool->Bool. f false) (\\x:Bool. if x then false else true)", 1);

    /* ---- λP demo ---- */
    printf("--- Vertex λP (dependent types) ---\n\n");
    current_vertex = CUBE_LAMBDA_P;

    /* Bool -> ★ is a valid type in λP: the type of predicates over Bool.
       It has sort □ (since ★:□ and the codomain is ★). */
    run("Bool -> *", 0);

    /* A concrete predicate: a family of types indexed by Bool.
       This is a term of type (Bool -> ★), i.e. a type-valued function. */
    run("\\b:Bool. if b then Bool else Bool", 0);

    /* A Pi type where the return type genuinely depends on the argument.
       The body uses b, so this is a proper dependent type. */
    run("Pi b:Bool. if b then Bool else Bool", 0);

    /* Applying the predicate λb:Bool. (if b then Bool else Bool)
       to true reduces to Bool. */
    run("(\\b:Bool. if b then Bool else Bool) true", 0);

    /* ---- λ2 demo ---- */
    printf("--- Vertex λ2 (System F — polymorphism) ---\n\n");
    current_vertex = CUBE_LAMBDA_2;

    /* The polymorphic identity: λA:★. λx:A. x
       Type: ΠA:★. A -> A  (terms depending on types) */
    run("\\A:*. \\x:A. x", 0);

    /* Applying the polymorphic identity to Bool then true */
    run("(\\A:*. \\x:A. x) Bool true", 0);

    /* Polymorphic const: λA:★. λB:★. λx:A. λy:B. x */
    run("\\A:*. \\B:*. \\x:A. \\y:B. x", 0);

    /* ---- λω demo ---- */
    printf("--- Vertex λω (type operators) ---\n\n");
    current_vertex = CUBE_LAMBDA_OMEGA;

    /* The identity on types: λA:★. A   has type ★ -> ★
       Requires (□,□): domain ★:□, codomain ★:□. */
    run("\\A:*. A", 0);

    /* A type-level const operator: λA:★. λB:★. A  has type ★ -> ★ -> ★ */
    run("\\A:*. \\B:*. A", 0);

    /* ---- λC demo ---- */
    printf("--- Vertex λC (Calculus of Constructions) ---\n\n");
    current_vertex = CUBE_LAMBDA_C;

    /* Polymorphic identity (from λ2) */
    run("\\A:*. \\x:A. x", 0);

    /* The type of the polymorphic identity */
    run("Pi A:*. A -> A", 0);

    /* A type operator applied in a dependent type:
       (λA:★. A -> A) Bool  reduces to  Bool -> Bool */
    run("(\\A:*. A -> A) Bool", 0);

    /* Dependent pair projection type — a Π with a type-operator in the domain.
       Pi A:*. Pi B:(A -> *). * 
       (The type of a "dependent eliminator shape") */
    run("Pi A:*. Pi B:(A -> *). *", 0);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv) {
    if (argc > 1) {
        /* :vertex flag */
        int i = 1;
        if (!strcmp(argv[i], "-vertex") || !strcmp(argv[i], "--vertex")) {
            i++;
            if (i >= argc) { fprintf(stderr, "Expected vertex name\n"); return 1; }
            int found = 0;
            for (int j = 0; j < N_VERTICES; j++) {
                if (!strcmp(argv[i], VERTICES[j].name)) {
                    current_vertex = VERTICES[j].v;
                    found = 1; break;
                }
            }
            if (!found) {
                fprintf(stderr, "Unknown vertex '%s'\n", argv[i]);
                return 1;
            }
            i++;
        }
        bool verbose = false;
        if (i < argc && !strcmp(argv[i], "-v")) { verbose = true; i++; }
        if (i < argc) {
            run(argv[i], verbose);
            return 0;
        }
    }

    demo();
    printf("--- REPL ---\n\n");
    repl();
    return 0;
}
