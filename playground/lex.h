#ifndef INCLUDE_playground_lex_h_
#define INCLUDE_playground_lex_h_

typedef enum {
  TOK_IDENT,
  TOK_LAMBDA,
  TOK_DOT,
  TOK_COLON,
  TOK_ARROW,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_EOF,
  TOK_TRUE,
  TOK_FALSE,
  TOK_IF,
  TOK_THEN,
  TOK_ELSE,
  TOK_BOOL
} TokenKind;

typedef struct {
  TokenKind kind;
  char text[64];
} Token;

typedef struct {
  const char *src;
  int pos;
  Token current;
  int has_current;
} Lexer;

void lexer_init(Lexer *lx, const char *src);
Token lexer_peek(Lexer *lx);
Token lexer_consume(Lexer *lx);
void expect(Lexer *lx, TokenKind kind, const char *what);

#endif // INCLUDE_playground_lex_h_
