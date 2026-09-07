#ifndef LEX_H
#define LEX_H

#include <stdbool.h>

typedef enum
{
  TOK_INVALID,
  TOK_IDENT,
  TOK_LAMBDA,
  TOK_PI,
  TOK_DOT,
  TOK_COLON,
  TOK_ARROW,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_STAR,
  TOK_BOX,
  TOK_TRUE,
  TOK_FALSE,
  TOK_BOOL,
  TOK_IF,
  TOK_THEN,
  TOK_ELSE,
  TOK_EOF,
  TOK_INDUCTIVE,
  TOK_WITH,
  TOK_END,
  TOK_RETURN,
  TOK_PIPE,
  TOK_ASSIGN,
  TOK_ELIM,
  TOK_EFFECT,
  TOK_IO,
  TOK_PURE,
  TOK_BIND,
  TOK_PUTCHAR,
  TOK_GETCHAR,
  TOK_IO_EXIT,
  TOK_PUTSTR,
  TOK_NAT_LIT,
  TOK_CHAR_LIT,
  TOK_STR_LIT,
} TokenKind;

typedef struct
{
  TokenKind kind;
  char text[128];
  int nat_val;
} Token;

typedef struct
{
  const char* src;
  int pos;
  Token current;
  int has_current;
} Lexer;

void
lexer_init(Lexer* lx, const char* src);
Token
lexer_peek(Lexer* lx);
Token
lexer_consume(Lexer* lx);
bool
expect(Lexer* lx, TokenKind kind, const char* what);

#endif /* LEX_H */
