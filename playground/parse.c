#include "parse.h"

#include <stdio.h>
#include <stdlib.h>

#include "lex.h"

static Type *parse_type(Lexer *lx);
static Term *parse_term(Lexer *lx);
static Term *parse_app(Lexer *lx);
static Term *parse_atom(Lexer *lx);

/* Type = AType (-> Type)?   (right-associative) */
static Type *parse_type(Lexer *lx) {
  Type *t;
  Token pk = lexer_peek(lx);
  if (pk.kind == TOK_BOOL) {
    lexer_consume(lx);
    t = mk_bool();
  } else if (pk.kind == TOK_LPAREN) {
    lexer_consume(lx);
    t = parse_type(lx);
    expect(lx, TOK_RPAREN, "')'");
  } else {
    fprintf(stderr, "Parse error: expected type, got '%s'\n", pk.text);
    exit(1);
  }
  /* Check for arrow */
  if (lexer_peek(lx).kind == TOK_ARROW) {
    lexer_consume(lx);
    Type *ret = parse_type(lx);
    t = mk_arr(t, ret);
  }
  return t;
}

/* Term = \x:T. t | if t then t else t | App */
static Term *parse_term(Lexer *lx) {
  Token pk = lexer_peek(lx);

  if (pk.kind == TOK_LAMBDA) {
    lexer_consume(lx);
    Token var = lexer_consume(lx);
    if (var.kind != TOK_IDENT) {
      fprintf(stderr,
              "Parse error: expected variable name after '\\', got '%s'\n",
              var.text);
      exit(1);
    }
    expect(lx, TOK_COLON, "':'");
    Type *ty = parse_type(lx);
    expect(lx, TOK_DOT, "'.'");
    Term *body = parse_term(lx);
    return mk_abs(var.text, ty, body);
  }

  if (pk.kind == TOK_IF) {
    lexer_consume(lx);
    Term *cond = parse_term(lx);
    expect(lx, TOK_THEN, "'then'");
    Term *tb = parse_term(lx);
    expect(lx, TOK_ELSE, "'else'");
    Term *eb = parse_term(lx);
    return mk_if(cond, tb, eb);
  }

  return parse_app(lx);
}

/* App = Atom Atom* (left-associative) */
static Term *parse_app(Lexer *lx) {
  Term *t = parse_atom(lx);
  while (1) {
    Token pk = lexer_peek(lx);
    /* An atom can start with: ident, true, false, ( */
    if (pk.kind != TOK_IDENT && pk.kind != TOK_TRUE && pk.kind != TOK_FALSE &&
        pk.kind != TOK_LPAREN)
      break;
    Term *arg = parse_atom(lx);
    t = mk_app(t, arg);
  }
  return t;
}

/* Atom = x | true | false | (term) */
static Term *parse_atom(Lexer *lx) {
  Token pk = lexer_peek(lx);
  if (pk.kind == TOK_IDENT) {
    lexer_consume(lx);
    return mk_var(pk.text);
  }
  if (pk.kind == TOK_TRUE) {
    lexer_consume(lx);
    return mk_true();
  }
  if (pk.kind == TOK_FALSE) {
    lexer_consume(lx);
    return mk_false();
  }
  if (pk.kind == TOK_LPAREN) {
    lexer_consume(lx);
    Term *t = parse_term(lx);
    expect(lx, TOK_RPAREN, "')'");
    return t;
  }
  fprintf(stderr, "Parse error: unexpected token '%s'\n", pk.text);
  exit(1);
}

Term *parse(const char *src) {
  Lexer lx;
  lexer_init(&lx, src);
  Term *t = parse_term(&lx);
  Token eof = lexer_peek(&lx);
  if (eof.kind != TOK_EOF) {
    fprintf(stderr, "Parse error: trailing tokens starting with '%s'\n",
            eof.text);
    exit(1);
  }
  return t;
}
