#include "parse.h"

#include <stdio.h>
#include <stdlib.h>

#include "lex.h"

static Expr *parse_expr(Lexer *lx);
static Expr *parse_app(Lexer *lx);
static Expr *parse_atom(Lexer *lx);

static Expr *parse_expr(Lexer *lx) {
  Token pk = lexer_peek(lx);

  /* λ x : T . body */
  if (pk.kind == TOK_LAMBDA) {
    lexer_consume(lx);
    Token var = lexer_consume(lx);
    if (var.kind != TOK_IDENT) {
      fprintf(stderr, "Parse error: expected variable after λ, got '%s'\n",
              var.text);
      exit(1);
    }
    expect(lx, TOK_COLON, "':'");
    Expr *ty = parse_expr(lx);
    expect(lx, TOK_DOT, "'.'");
    Expr *body = parse_expr(lx);
    return e_lam(var.text, ty, body);
  }

  /* Pi x : T . body  (or  Π x : T . body) */
  if (pk.kind == TOK_PI) {
    lexer_consume(lx);
    Token var = lexer_consume(lx);
    if (var.kind != TOK_IDENT) {
      fprintf(stderr, "Parse error: expected variable after Π, got '%s'\n",
              var.text);
      exit(1);
    }
    expect(lx, TOK_COLON, "':'");
    Expr *ty = parse_expr(lx);
    expect(lx, TOK_DOT, "'.'");
    Expr *body = parse_expr(lx);
    return e_pi(var.text, ty, body);
  }

  if (pk.kind == TOK_IF) {
    lexer_consume(lx);
    Expr *cond = parse_expr(lx);
    expect(lx, TOK_THEN, "'then'");
    Expr *tb = parse_expr(lx);
    expect(lx, TOK_ELSE, "'else'");
    Expr *eb = parse_expr(lx);
    return e_if(cond, tb, eb);
  }

  /* app ( -> expr )* */
  Expr *lhs = parse_app(lx);
  if (lexer_peek(lx).kind == TOK_ARROW) {
    lexer_consume(lx);
    Expr *rhs = parse_expr(lx); /* recursive → right-assoc */
    /* Desugar: use a fresh binder name that won't appear in rhs */
    return e_pi("_", lhs, rhs);
  }
  return lhs;
}

/*
 * app ::= atom+
 */
static Expr *parse_app(Lexer *lx) {
  Expr *t = parse_atom(lx);
  while (1) {
    Token pk = lexer_peek(lx);
    /* atom starters */
    if (pk.kind != TOK_IDENT && pk.kind != TOK_TRUE && pk.kind != TOK_FALSE &&
        pk.kind != TOK_BOOL && pk.kind != TOK_STAR && pk.kind != TOK_BOX &&
        pk.kind != TOK_LPAREN)
      break;
    Expr *arg = parse_atom(lx);
    t = e_app(t, arg);
  }
  return t;
}

/*
 * atom ::= x | * | □ | Bool | true | false | '(' expr ')'
 */
static Expr *parse_atom(Lexer *lx) {
  Token pk = lexer_peek(lx);
  switch (pk.kind) {
  case TOK_IDENT:
    lexer_consume(lx);
    return e_var(pk.text);
  case TOK_STAR:
    lexer_consume(lx);
    return e_star();
  case TOK_BOX:
    lexer_consume(lx);
    return e_box();
  case TOK_BOOL:
    lexer_consume(lx);
    return e_bool();
  case TOK_TRUE:
    lexer_consume(lx);
    return e_true();
  case TOK_FALSE:
    lexer_consume(lx);
    return e_false();
  case TOK_LPAREN: {
    lexer_consume(lx);
    Expr *e = parse_expr(lx);
    expect(lx, TOK_RPAREN, "')'");
    return e;
  }
  default:
    fprintf(stderr, "Parse error: unexpected token '%s'\n", pk.text);
    exit(1);
  }
}

Expr *parse(const char *src) {
  Lexer lx;
  lexer_init(&lx, src);
  Expr *e = parse_expr(&lx);
  Token eof = lexer_peek(&lx);
  if (eof.kind != TOK_EOF) {
    fprintf(stderr, "Parse error: trailing tokens starting with '%s'\n",
            eof.text);
    exit(1);
  }
  return e;
}
