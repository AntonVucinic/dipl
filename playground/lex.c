#include "lex.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lexer_init(Lexer *lx, const char *src) {
  lx->src = src;
  lx->pos = 0;
  lx->has_current = 0;
}

static void skip_ws(Lexer *lx) {
  while (lx->src[lx->pos] && isspace((unsigned char)lx->src[lx->pos]))
    lx->pos++;
}

Token lexer_next(Lexer *lx) {
  skip_ws(lx);
  Token tok = {0};
  char c = lx->src[lx->pos];

  if (!c) {
    tok.kind = TOK_EOF;
    strcpy(tok.text, "<eof>");
    return tok;
  }

  {
    unsigned char uc = (unsigned char)c;
    if (c == '\\' || uc == 0xCE) { /* lambda or Unicode λ (0xCE 0xBB) */
      if (uc == 0xCE && (unsigned char)lx->src[lx->pos + 1] == 0xBB)
        lx->pos += 2;
      else
        lx->pos++;
      tok.kind = TOK_LAMBDA;
      strcpy(tok.text, "\\");
      return tok;
    }
  }
  if (c == '.') {
    lx->pos++;
    tok.kind = TOK_DOT;
    strcpy(tok.text, ".");
    return tok;
  }
  if (c == ':') {
    lx->pos++;
    /* Check for -> as :  -- no, -> is separate */
    tok.kind = TOK_COLON;
    strcpy(tok.text, ":");
    return tok;
  }
  if (c == '(') {
    lx->pos++;
    tok.kind = TOK_LPAREN;
    strcpy(tok.text, "(");
    return tok;
  }
  if (c == ')') {
    lx->pos++;
    tok.kind = TOK_RPAREN;
    strcpy(tok.text, ")");
    return tok;
  }
  if (c == '-' && lx->src[lx->pos + 1] == '>') {
    lx->pos += 2;
    tok.kind = TOK_ARROW;
    strcpy(tok.text, "->");
    return tok;
  }

  if (isalpha((unsigned char)c) || c == '_') {
    int start = lx->pos;
    while (isalnum((unsigned char)lx->src[lx->pos]) ||
           lx->src[lx->pos] == '_' || lx->src[lx->pos] == '\'')
      lx->pos++;
    int len = lx->pos - start;
    if (len >= 63)
      len = 63;
    strncpy(tok.text, lx->src + start, len);
    tok.text[len] = '\0';

    if (!strcmp(tok.text, "true")) {
      tok.kind = TOK_TRUE;
      return tok;
    }
    if (!strcmp(tok.text, "false")) {
      tok.kind = TOK_FALSE;
      return tok;
    }
    if (!strcmp(tok.text, "if")) {
      tok.kind = TOK_IF;
      return tok;
    }
    if (!strcmp(tok.text, "then")) {
      tok.kind = TOK_THEN;
      return tok;
    }
    if (!strcmp(tok.text, "else")) {
      tok.kind = TOK_ELSE;
      return tok;
    }
    if (!strcmp(tok.text, "Bool")) {
      tok.kind = TOK_BOOL;
      return tok;
    }
    tok.kind = TOK_IDENT;
    return tok;
  }

  fprintf(stderr, "Lexer error: unexpected character '%c'\n", c);
  exit(1);
}

Token lexer_peek(Lexer *lx) {
  if (!lx->has_current) {
    lx->current = lexer_next(lx);
    lx->has_current = 1;
  }
  return lx->current;
}

Token lexer_consume(Lexer *lx) {
  Token t = lexer_peek(lx);
  lx->has_current = 0;
  return t;
}

void expect(Lexer *lx, TokenKind kind, const char *what) {
  Token t = lexer_consume(lx);
  if (t.kind != kind) {
    fprintf(stderr, "Parse error: expected %s, got '%s'\n", what, t.text);
    exit(1);
  }
}
