#ifndef LEX_H
#define LEX_H

typedef enum {
    TOK_IDENT,
    TOK_LAMBDA,   /* \  or  λ  */
    TOK_PI,       /* Pi  or  Π  */
    TOK_DOT,      /* .  */
    TOK_COLON,    /* :  */
    TOK_ARROW,    /* -> or → */
    TOK_LPAREN,   /* (  */
    TOK_RPAREN,   /* )  */
    TOK_STAR,     /* *  or  ★  */
    TOK_BOX,      /* □  or  BOX  */
    TOK_TRUE,
    TOK_FALSE,
    TOK_BOOL,
    TOK_IF,
    TOK_THEN,
    TOK_ELSE,
    TOK_EOF,
} TokenKind;

typedef struct {
    TokenKind kind;
    char      text[128];
} Token;

typedef struct {
    const char *src;
    int         pos;
    Token       current;
    int         has_current;
} Lexer;

void  lexer_init   (Lexer *lx, const char *src);
Token lexer_peek   (Lexer *lx);
Token lexer_consume(Lexer *lx);
void  expect       (Lexer *lx, TokenKind kind, const char *what);

#endif /* LEX_H */
