#include <stdbool.h>
#define _POSIX_C_SOURCE 200809L
#include "lex.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

void
lexer_init(Lexer* lx, const char* src)
{
  lx->src = src;
  lx->pos = 0;
  lx->has_current = 0;
}

static void
skip_ws(Lexer* lx)
{
  while (lx->src[lx->pos] && isspace((unsigned char)lx->src[lx->pos]))
    lx->pos++;
}

static int
match_utf8(Lexer* lx, const unsigned char* seq, int len)
{
  for (int i = 0; i < len; i++)
    if ((unsigned char)lx->src[lx->pos + i] != seq[i])
      return 0;
  return len;
}

static Token
lexer_next(Lexer* lx)
{
  skip_ws(lx);
  Token tok;
  memset(&tok, 0, sizeof(tok));
  unsigned char c = (unsigned char)lx->src[lx->pos];

  if (c == '\0') {
    tok.kind = TOK_EOF;
    strcpy(tok.text, "<eof>");
    return tok;
  }

  static const unsigned char UTF_LAMBDA[] = { 0xCE, 0xBB };
  static const unsigned char UTF_PI[] = { 0xCE, 0xA0 };
  static const unsigned char UTF_STAR[] = { 0xE2, 0x98, 0x85 };
  static const unsigned char UTF_BOX[] = { 0xE2, 0x96, 0xA1 };
  static const unsigned char UTF_ARROW[] = { 0xE2, 0x86, 0x92 };

  int n;
  if ((n = match_utf8(lx, UTF_LAMBDA, 2))) {
    lx->pos += n;
    tok.kind = TOK_LAMBDA;
    strcpy(tok.text, "λ");
    return tok;
  }
  if ((n = match_utf8(lx, UTF_PI, 2))) {
    lx->pos += n;
    tok.kind = TOK_PI;
    strcpy(tok.text, "Π");
    return tok;
  }
  if ((n = match_utf8(lx, UTF_STAR, 3))) {
    lx->pos += n;
    tok.kind = TOK_STAR;
    strcpy(tok.text, "★");
    return tok;
  }
  if ((n = match_utf8(lx, UTF_BOX, 3))) {
    lx->pos += n;
    tok.kind = TOK_BOX;
    strcpy(tok.text, "□");
    return tok;
  }
  if ((n = match_utf8(lx, UTF_ARROW, 3))) {
    lx->pos += n;
    tok.kind = TOK_ARROW;
    strcpy(tok.text, "→");
    return tok;
  }

  if (c == '\\') {
    lx->pos++;
    tok.kind = TOK_LAMBDA;
    strcpy(tok.text, "\\");
    return tok;
  }
  if (c == '.') {
    lx->pos++;
    tok.kind = TOK_DOT;
    strcpy(tok.text, ".");
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
  if (c == '*') {
    lx->pos++;
    tok.kind = TOK_STAR;
    strcpy(tok.text, "*");
    return tok;
  }
  if (c == '|') {
    lx->pos++;
    tok.kind = TOK_PIPE;
    strcpy(tok.text, "|");
    return tok;
  }

  if (c == ':' && (unsigned char)lx->src[lx->pos + 1] == '=') {
    lx->pos += 2;
    tok.kind = TOK_ASSIGN;
    strcpy(tok.text, ":=");
    return tok;
  }
  if (c == ':') {
    lx->pos++;
    tok.kind = TOK_COLON;
    strcpy(tok.text, ":");
    return tok;
  }

  if (c == '-' && (unsigned char)lx->src[lx->pos + 1] == '>') {
    lx->pos += 2;
    tok.kind = TOK_ARROW;
    strcpy(tok.text, "->");
    return tok;
  }

  if (isalpha(c) || c == '_') {
    int start = lx->pos;
    while (isalnum((unsigned char)lx->src[lx->pos]) ||
           lx->src[lx->pos] == '_' || lx->src[lx->pos] == '\'')
      lx->pos++;
    int len = lx->pos - start;
    if (len > 127)
      len = 127;
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
    if (!strcmp(tok.text, "Bool")) {
      tok.kind = TOK_BOOL;
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
    if (!strcmp(tok.text, "Pi")) {
      tok.kind = TOK_PI;
      return tok;
    }
    if (!strcmp(tok.text, "BOX") || !strcmp(tok.text, "Box")) {
      tok.kind = TOK_BOX;
      return tok;
    }
    if (!strcmp(tok.text, "inductive")) {
      tok.kind = TOK_INDUCTIVE;
      return tok;
    }
    if (!strcmp(tok.text, "with")) {
      tok.kind = TOK_WITH;
      return tok;
    }
    if (!strcmp(tok.text, "end")) {
      tok.kind = TOK_END;
      return tok;
    }
    if (!strcmp(tok.text, "return")) {
      tok.kind = TOK_RETURN;
      return tok;
    }
    if (!strcmp(tok.text, "elim")) {
      tok.kind = TOK_ELIM;
      return tok;
    }
    if (!strcmp(tok.text, "effect")) {
      tok.kind = TOK_EFFECT;
      return tok;
    }
    if (!strcmp(tok.text, "IO")) {
      tok.kind = TOK_IO;
      return tok;
    }
    if (!strcmp(tok.text, "pure")) {
      tok.kind = TOK_PURE;
      return tok;
    }
    if (!strcmp(tok.text, "bind")) {
      tok.kind = TOK_BIND;
      return tok;
    }
    if (!strcmp(tok.text, "putChar")) {
      tok.kind = TOK_PUTCHAR;
      return tok;
    }
    if (!strcmp(tok.text, "getChar")) {
      tok.kind = TOK_GETCHAR;
      return tok;
    }
    if (!strcmp(tok.text, "exit")) {
      tok.kind = TOK_IO_EXIT;
      return tok;
    }
    if (!strcmp(tok.text, "putStr")) {
      tok.kind = TOK_PUTSTR;
      return tok;
    }
    tok.kind = TOK_IDENT;
    return tok;
  }

  if (isdigit(c)) {
    int val = 0;
    while (isdigit((unsigned char)lx->src[lx->pos])) {
      val = val * 10 + (lx->src[lx->pos] - '0');
      lx->pos++;
    }
    tok.kind = TOK_NAT_LIT;
    tok.nat_val = val;
    snprintf(tok.text, sizeof(tok.text), "%d", val);
    return tok;
  }

  if (c == '\'') {
    lx->pos++;
    int ch;
    if (lx->src[lx->pos] == '\\') {
      lx->pos++;
      switch (lx->src[lx->pos]) {
        case 'n':
          ch = '\n';
          break;
        case 't':
          ch = '\t';
          break;
        case 'r':
          ch = '\r';
          break;
        case '0':
          ch = '\0';
          break;
        case '\\':
          ch = '\\';
          break;
        case '\'':
          ch = '\'';
          break;
        default:
          ch = (unsigned char)lx->src[lx->pos];
          break;
      }
      lx->pos++;
    } else if (lx->src[lx->pos] && lx->src[lx->pos] != '\'') {
      ch = (unsigned char)lx->src[lx->pos++];
    } else {
      fprintf(stderr, "Lexer error: empty character literal\n");
      tok.kind = TOK_INVALID;
      return tok;
    }
    if (lx->src[lx->pos] != '\'') {
      fprintf(stderr, "Lexer error: unterminated character literal\n");
      tok.kind = TOK_INVALID;
      return tok;
    }
    lx->pos++;
    tok.kind = TOK_CHAR_LIT;
    tok.nat_val = ch;
    snprintf(
      tok.text, sizeof(tok.text), "'%c'", (ch >= 32 && ch < 127) ? ch : '?');
    return tok;
  }

  if (c == '"') {
    lx->pos++;
    int out = 0;
    while (lx->src[lx->pos] && lx->src[lx->pos] != '"') {
      char ch;
      if (lx->src[lx->pos] == '\\') {
        lx->pos++;
        switch (lx->src[lx->pos]) {
          case 'n':
            ch = '\n';
            break;
          case 't':
            ch = '\t';
            break;
          case 'r':
            ch = '\r';
            break;
          case '0':
            ch = '\0';
            break;
          case '\\':
            ch = '\\';
            break;
          case '"':
            ch = '"';
            break;
          default:
            ch = lx->src[lx->pos];
            break;
        }
      } else {
        ch = lx->src[lx->pos];
      }
      if (out < 126)
        tok.text[out++] = ch;
      lx->pos++;
    }
    if (lx->src[lx->pos] != '"') {
      fprintf(stderr, "Lexer error: unterminated string literal\n");
      tok.kind = TOK_INVALID;
      return tok;
    }
    lx->pos++;
    tok.text[out] = '\0';
    tok.kind = TOK_STR_LIT;
    return tok;
  }

  fprintf(stderr, "Lexer error: unexpected character '%c'\n", c);
  tok.kind = TOK_INVALID;
  return tok;
}

Token
lexer_peek(Lexer* lx)
{
  if (!lx->has_current) {
    lx->current = lexer_next(lx);
    lx->has_current = 1;
  }
  return lx->current;
}

Token
lexer_consume(Lexer* lx)
{
  Token t = lexer_peek(lx);
  lx->has_current = 0;
  return t;
}

bool
expect(Lexer* lx, TokenKind kind, const char* what)
{
  Token t = lexer_consume(lx);
  if (t.kind == TOK_INVALID)
    return false;
  if (t.kind != kind) {
    fprintf(stderr, "Parse error: expected %s, got '%s'\n", what, t.text);
    return false;
  }
  return true;
}
