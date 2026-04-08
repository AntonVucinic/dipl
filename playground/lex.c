#define _POSIX_C_SOURCE 200809L
#include "lex.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lexer_init(Lexer *lx, const char *src) {
    lx->src         = src;
    lx->pos         = 0;
    lx->has_current = 0;
}

static void skip_ws(Lexer *lx) {
    while (lx->src[lx->pos] &&
           isspace((unsigned char)lx->src[lx->pos]))
        lx->pos++;
}

/*
 * Try to match a UTF-8 sequence starting at lx->pos.
 * Returns the byte-length of the matched sequence, or 0 on failure.
 */
static int match_utf8(Lexer *lx, const unsigned char *seq, int len) {
    for (int i = 0; i < len; i++)
        if ((unsigned char)lx->src[lx->pos + i] != seq[i]) return 0;
    return len;
}

static Token lexer_next(Lexer *lx) {
    skip_ws(lx);
    Token tok;
    memset(&tok, 0, sizeof(tok));

    unsigned char c = (unsigned char)lx->src[lx->pos];

    if (c == '\0') {
        tok.kind = TOK_EOF;
        strcpy(tok.text, "<eof>");
        return tok;
    }

    /* ---- Unicode multi-byte sequences ---- */

    /* λ  U+03BB  = 0xCE 0xBB */
    static const unsigned char UTF_LAMBDA[] = {0xCE, 0xBB};
    /* Π  U+03A0  = 0xCE 0xA0 */
    static const unsigned char UTF_PI[]     = {0xCE, 0xA0};
    /* ★  U+2605  = 0xE2 0x98 0x85 */
    static const unsigned char UTF_STAR[]   = {0xE2, 0x98, 0x85};
    /* □  U+25A1  = 0xE2 0x96 0xA1 */
    static const unsigned char UTF_BOX[]    = {0xE2, 0x96, 0xA1};
    /* →  U+2192  = 0xE2 0x86 0x92 */
    static const unsigned char UTF_ARROW[]  = {0xE2, 0x86, 0x92};

    int n;
    if ((n = match_utf8(lx, UTF_LAMBDA, 2))) {
        lx->pos += n; tok.kind = TOK_LAMBDA; strcpy(tok.text, "λ"); return tok;
    }
    if ((n = match_utf8(lx, UTF_PI, 2))) {
        lx->pos += n; tok.kind = TOK_PI;     strcpy(tok.text, "Π"); return tok;
    }
    if ((n = match_utf8(lx, UTF_STAR, 3))) {
        lx->pos += n; tok.kind = TOK_STAR;   strcpy(tok.text, "★"); return tok;
    }
    if ((n = match_utf8(lx, UTF_BOX, 3))) {
        lx->pos += n; tok.kind = TOK_BOX;    strcpy(tok.text, "□"); return tok;
    }
    if ((n = match_utf8(lx, UTF_ARROW, 3))) {
        lx->pos += n; tok.kind = TOK_ARROW;  strcpy(tok.text, "→"); return tok;
    }

    /* ---- ASCII single / double char ---- */
    if (c == '\\') {
        lx->pos++; tok.kind = TOK_LAMBDA; strcpy(tok.text, "\\"); return tok;
    }
    if (c == '.') {
        lx->pos++; tok.kind = TOK_DOT;    strcpy(tok.text, ".");  return tok;
    }
    if (c == ':') {
        lx->pos++; tok.kind = TOK_COLON;  strcpy(tok.text, ":");  return tok;
    }
    if (c == '(') {
        lx->pos++; tok.kind = TOK_LPAREN; strcpy(tok.text, "(");  return tok;
    }
    if (c == ')') {
        lx->pos++; tok.kind = TOK_RPAREN; strcpy(tok.text, ")");  return tok;
    }
    if (c == '*') {
        lx->pos++; tok.kind = TOK_STAR;   strcpy(tok.text, "*");  return tok;
    }
    if (c == '-' && (unsigned char)lx->src[lx->pos + 1] == '>') {
        lx->pos += 2; tok.kind = TOK_ARROW; strcpy(tok.text, "->"); return tok;
    }

    /* ---- Identifiers and keywords ---- */
    if (isalpha(c) || c == '_') {
        int start = lx->pos;
        while (isalnum((unsigned char)lx->src[lx->pos]) ||
               lx->src[lx->pos] == '_' ||
               lx->src[lx->pos] == '\'')
            lx->pos++;
        int len = lx->pos - start;
        if (len > 127) len = 127;
        strncpy(tok.text, lx->src + start, len);
        tok.text[len] = '\0';

        if (!strcmp(tok.text, "true"))  { tok.kind = TOK_TRUE;   return tok; }
        if (!strcmp(tok.text, "false")) { tok.kind = TOK_FALSE;  return tok; }
        if (!strcmp(tok.text, "Bool"))  { tok.kind = TOK_BOOL;   return tok; }
        if (!strcmp(tok.text, "if"))    { tok.kind = TOK_IF;     return tok; }
        if (!strcmp(tok.text, "then"))  { tok.kind = TOK_THEN;   return tok; }
        if (!strcmp(tok.text, "else"))  { tok.kind = TOK_ELSE;   return tok; }
        if (!strcmp(tok.text, "Pi"))    { tok.kind = TOK_PI;     return tok; }
        if (!strcmp(tok.text, "BOX") ||
            !strcmp(tok.text, "Box"))   { tok.kind = TOK_BOX;    return tok; }
        tok.kind = TOK_IDENT;
        return tok;
    }

    fprintf(stderr, "Lexer error: unexpected character 0x%02x '%c'\n", c, c);
    exit(1);
}

Token lexer_peek(Lexer *lx) {
    if (!lx->has_current) {
        lx->current     = lexer_next(lx);
        lx->has_current = 1;
    }
    return lx->current;
}

Token lexer_consume(Lexer *lx) {
    Token t         = lexer_peek(lx);
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
