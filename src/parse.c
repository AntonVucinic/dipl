#define _POSIX_C_SOURCE 200809L
#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"
#include "subst.h"

static Expr*
parse_expr(Lexer* lx);
static Expr*
parse_app(Lexer* lx);
static Expr*
parse_atom(Lexer* lx);

static Expr*
nat_lit(int n)
{
  Expr* r = ind_lookup_con("zero", NULL) ? e_con("zero") : e_var("zero");
  for (int i = 0; i < n; i++) {
    Expr* s = ind_lookup_con("succ", NULL) ? e_con("succ") : e_var("succ");
    r = e_app(s, r);
  }
  return r;
}

static Expr*
str_lit(const char* s)
{
  Expr* nil = ind_lookup_con("nil", NULL) ? e_con("nil") : e_var("nil");
  Expr* cons = ind_lookup_con("cons", NULL) ? e_con("cons") : e_var("cons");
  Expr* nat = ind_lookup("Nat") ? e_ind("Nat") : e_var("Nat");
  Expr* result = e_app(nil, nat);
  int len = (int)strlen(s);
  for (int i = len - 1; i >= 0; i--)
    result = e_app(e_app(e_app(expr_clone(cons), expr_clone(nat)),
                         nat_lit((unsigned char)s[i])),
                   result);
  return result;
}

static Expr*
wrap_branch_subst(char** binders, int n_b, Expr* body, const Expr* pi_type)
{
  Expr** anns = malloc(n_b * sizeof(Expr*));
  const Expr* cur = pi_type;
  for (int i = 0; i < n_b; i++) {
    if (cur && cur->tag == E_PI) {
      anns[i] = expr_clone(cur->type);
      Expr* next = subst(cur->binder, e_var(binders[i]), cur->body);
      cur = next;
    } else {
      anns[i] = e_var("_");
    }
  }
  for (int i = n_b - 1; i >= 0; i--)
    body = e_lam(binders[i], anns[i], body);
  free(anns);
  return body;
}

static Expr*
elim_branch_type(IndDef* ind, int ci)
{
  const Expr* cur = ind->elim_type;
  for (int i = 0; i < ind->n_params; i++) {
    if (cur->tag != E_PI)
      return NULL;
    cur = cur->body;
  }
  if (cur->tag != E_PI)
    return NULL;
  cur = cur->body;
  for (int i = 0; i < ci; i++) {
    if (cur->tag != E_PI)
      return NULL;
    cur = cur->body;
  }
  if (cur->tag != E_PI)
    return NULL;
  return expr_clone(cur->type);
}

static Expr*
parse_elim(Lexer* lx)
{
  Expr* scrutinee = parse_expr(lx);
  if (!scrutinee)
    return NULL;

  if (lexer_peek(lx).kind != TOK_RETURN) {
    fprintf(stderr, "Parse error: expected 'return' after elim scrutinee\n");
    return NULL;
  }
  lexer_consume(lx);
  Expr* motive = parse_expr(lx);
  if (!motive)
    return NULL;

  if (lexer_peek(lx).kind != TOK_WITH) {
    fprintf(stderr, "Parse error: expected 'with' after elim motive\n");
    return NULL;
  }
  lexer_consume(lx);

  int cap = 8, n_branches = 0;
  char** con_names = malloc(cap * sizeof(char*));
  Expr** branches = malloc(cap * sizeof(Expr*));

  while (lexer_peek(lx).kind == TOK_PIPE) {
    lexer_consume(lx);

    Token con_tok = lexer_consume(lx);
    if (con_tok.kind != TOK_IDENT) {
      fprintf(stderr, "Parse error: expected constructor name in branch\n");
      free(con_names);
      free(branches);
      return NULL;
    }

    int b_cap = 8, n_b = 0;
    char** binders = malloc(b_cap * sizeof(char*));
    while (lexer_peek(lx).kind == TOK_IDENT) {
      Token bt = lexer_consume(lx);
      if (n_b == b_cap) {
        b_cap *= 2;
        binders = realloc(binders, b_cap * sizeof(char*));
      }
      binders[n_b++] = strdup(bt.text);
    }

    if (!expect(lx, TOK_THEN, "'then'")) {
      free(binders);
      free(con_names);
      free(branches);
      return NULL;
    }

    Expr* body = parse_expr(lx);
    if (!body) {
      free(binders);
      free(con_names);
      free(branches);
      return NULL;
    }

    int ci_tmp;
    IndDef* ind_tmp = ind_lookup_con(con_tok.text, &ci_tmp);
    Expr* branch_ty_raw = ind_tmp ? elim_branch_type(ind_tmp, ci_tmp) : NULL;
    Expr* branch_ty = branch_ty_raw;
    if (branch_ty && motive) {
      branch_ty = subst("P", motive, branch_ty);
    }
    body = wrap_branch_subst(binders, n_b, body, branch_ty ? branch_ty : NULL);
    for (int i = 0; i < n_b; i++)
      free(binders[i]);
    free(binders);

    if (n_branches == cap) {
      cap *= 2;
      con_names = realloc(con_names, cap * sizeof(char*));
      branches = realloc(branches, cap * sizeof(Expr*));
    }
    con_names[n_branches] = strdup(con_tok.text);
    branches[n_branches] = body;
    n_branches++;
  }

  if (lexer_peek(lx).kind != TOK_END) {
    fprintf(stderr, "Parse error: expected 'end' to close elim\n");
    free(con_names);
    free(branches);
    return NULL;
  }
  lexer_consume(lx);

  if (n_branches == 0) {
    fprintf(stderr, "Parse error: elim has no branches\n");
    free(con_names);
    free(branches);
    return NULL;
  }

  int dummy;
  IndDef* ind = ind_lookup_con(con_names[0], &dummy);
  if (!ind) {
    fprintf(
      stderr, "Parse error: '%s' is not a known constructor\n", con_names[0]);
    free(con_names);
    free(branches);
    return NULL;
  }

  if (n_branches != ind->n_con) {
    fprintf(stderr,
            "Parse error: elim for '%s' needs %d branches, got %d\n",
            ind->name,
            ind->n_con,
            n_branches);
    free(con_names);
    free(branches);
    return NULL;
  }

  Expr** ordered = calloc(ind->n_con, sizeof(Expr*));
  for (int j = 0; j < n_branches; j++) {
    int ci;
    IndDef* owner = ind_lookup_con(con_names[j], &ci);
    if (owner == ind)
      ordered[ci] = branches[j];
  }
  for (int i = 0; i < ind->n_con; i++) {
    if (!ordered[i]) {
      fprintf(
        stderr, "Parse error: missing branch for '%s'\n", ind->cons[i].name);
      free(ordered);
      free(con_names);
      free(branches);
      return NULL;
    }
  }

  char elim_name[128];
  snprintf(elim_name, sizeof(elim_name), "%s_elim", ind->name);
  Expr* result = e_elim(elim_name);
  for (int i = 0; i < ind->n_params; i++)
    result = e_app(result, e_var(ind->params[i].name));
  result = e_app(result, motive);
  for (int i = 0; i < ind->n_con; i++)
    result = e_app(result, ordered[i]);
  result = e_app(result, scrutinee);

  for (int i = 0; i < n_branches; i++)
    free(con_names[i]);
  free(con_names);
  free(branches);
  free(ordered);
  return result;
}

static Expr*
parse_expr(Lexer* lx)
{
  Token pk = lexer_peek(lx);
  if (pk.kind == TOK_INVALID)
    return NULL;

  if (pk.kind == TOK_LAMBDA) {
    lexer_consume(lx);
    Token var = lexer_consume(lx);
    if (var.kind != TOK_IDENT) {
      fprintf(
        stderr, "Parse error: expected variable after λ, got '%s'\n", var.text);
      return NULL;
    }
    if (!expect(lx, TOK_COLON, "':'"))
      return NULL;
    Expr* ty = parse_expr(lx);
    if (!ty)
      return NULL;
    if (!expect(lx, TOK_DOT, "'.'"))
      return NULL;
    Expr* body = parse_expr(lx);
    if (!body)
      return NULL;
    return e_lam(var.text, ty, body);
  }

  if (pk.kind == TOK_PI) {
    lexer_consume(lx);
    Token var = lexer_consume(lx);
    if (var.kind != TOK_IDENT) {
      fprintf(
        stderr, "Parse error: expected variable after Π, got '%s'\n", var.text);
      return NULL;
    }
    if (!expect(lx, TOK_COLON, "':'"))
      return NULL;
    Expr* ty = parse_expr(lx);
    if (!ty)
      return NULL;
    if (!expect(lx, TOK_DOT, "'.'"))
      return NULL;
    Expr* body = parse_expr(lx);
    if (!body)
      return NULL;
    return e_pi(var.text, ty, body);
  }

  if (pk.kind == TOK_IF) {
    lexer_consume(lx);
    Expr* cond = parse_expr(lx);
    if (!cond)
      return NULL;
    if (!expect(lx, TOK_THEN, "'then'"))
      return NULL;
    Expr* tb = parse_expr(lx);
    if (!tb)
      return NULL;
    if (!expect(lx, TOK_ELSE, "'else'"))
      return NULL;
    Expr* eb = parse_expr(lx);
    if (!eb)
      return NULL;
    return e_if(cond, tb, eb);
  }

  if (pk.kind == TOK_ELIM) {
    lexer_consume(lx);
    return parse_elim(lx);
  }

  Expr* lhs = parse_app(lx);
  if (!lhs)
    return NULL;
  if (lexer_peek(lx).kind == TOK_ARROW) {
    lexer_consume(lx);
    Expr* rhs = parse_expr(lx);
    if (!rhs)
      return NULL;
    return e_pi("_", lhs, rhs);
  }
  return lhs;
}

static Expr*
parse_app(Lexer* lx)
{
  Expr* t = parse_atom(lx);
  if (!t)
    return NULL;
  while (1) {
    Token pk = lexer_peek(lx);
    if (pk.kind == TOK_INVALID)
      return NULL;
    if (pk.kind != TOK_IDENT && pk.kind != TOK_TRUE && pk.kind != TOK_FALSE &&
        pk.kind != TOK_BOOL && pk.kind != TOK_STAR && pk.kind != TOK_BOX &&
        pk.kind != TOK_IO && pk.kind != TOK_PURE && pk.kind != TOK_BIND &&
        pk.kind != TOK_PUTCHAR && pk.kind != TOK_GETCHAR &&
        pk.kind != TOK_IO_EXIT && pk.kind != TOK_PUTSTR &&
        pk.kind != TOK_NAT_LIT && pk.kind != TOK_CHAR_LIT &&
        pk.kind != TOK_STR_LIT && pk.kind != TOK_LPAREN)
      break;
    Expr* arg = parse_atom(lx);
    if (!arg)
      return NULL;
    t = e_app(t, arg);
  }
  return t;
}

static Expr*
parse_atom(Lexer* lx)
{
  Token pk = lexer_peek(lx);
  switch (pk.kind) {
    case TOK_INVALID:
      return NULL;
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
    case TOK_IO:
      lexer_consume(lx);
      return e_io_ty();
    case TOK_PURE:
      lexer_consume(lx);
      return e_io_pure();
    case TOK_BIND:
      lexer_consume(lx);
      return e_io_bind();
    case TOK_PUTCHAR:
      lexer_consume(lx);
      return e_io_putchar();
    case TOK_GETCHAR:
      lexer_consume(lx);
      return e_io_getchar();
    case TOK_IO_EXIT:
      lexer_consume(lx);
      return e_io_exit();
    case TOK_PUTSTR:
      lexer_consume(lx);
      return e_io_putstr();
    case TOK_NAT_LIT: {
      int v = pk.nat_val;
      lexer_consume(lx);
      return nat_lit(v);
    }
    case TOK_CHAR_LIT: {
      int v = pk.nat_val;
      lexer_consume(lx);
      return nat_lit(v);
    }
    case TOK_STR_LIT: {
      lexer_consume(lx);
      return str_lit(pk.text);
    }
    case TOK_LPAREN: {
      lexer_consume(lx);
      Expr* e = parse_expr(lx);
      if (!e)
        return NULL;
      if (!expect(lx, TOK_RPAREN, "')'"))
        return NULL;
      return e;
    }
    default:
      fprintf(stderr, "Parse error: unexpected token '%s'\n", pk.text);
      return NULL;
  }
}

Expr*
parse(const char* src)
{
  Lexer lx;
  lexer_init(&lx, src);
  Expr* e = parse_expr(&lx);
  if (!e)
    return NULL;
  Token eof = lexer_peek(&lx);
  if (eof.kind == TOK_INVALID)
    return NULL;
  if (eof.kind != TOK_EOF) {
    fprintf(
      stderr, "Parse error: trailing tokens starting with '%s'\n", eof.text);
    return NULL;
  }
  return e;
}

int
is_effect_decl(const char* src)
{
  while (*src == ' ' || *src == '\t')
    src++;
  return strncmp(src, "effect", 6) == 0 &&
         (src[6] == ' ' || src[6] == '\t' || src[6] == '\0');
}

Expr*
parse_effect(const char* src, char** name_out)
{
  Lexer lx;
  lexer_init(&lx, src);

  if (!expect(&lx, TOK_EFFECT, "'effect'"))
    return NULL;

  Token name_tok = lexer_consume(&lx);
  if (name_tok.kind != TOK_IDENT) {
    fprintf(stderr, "Parse error: expected name after 'effect'\n");
    return NULL;
  }

  if (!expect(&lx, TOK_ASSIGN, "':='"))
    return NULL;

  Expr* body = parse_expr(&lx);
  if (!body)
    return NULL;

  Token eof = lexer_peek(&lx);
  if (eof.kind != TOK_EOF) {
    fprintf(stderr, "Parse error: trailing tokens in effect declaration\n");
    return NULL;
  }

  if (name_out)
    *name_out = strdup(name_tok.text);
  return body;
}

int
is_inductive_decl(const char* src)
{
  while (*src == ' ' || *src == '\t')
    src++;
  return strncmp(src, "inductive", 9) == 0 &&
         (src[9] == ' ' || src[9] == '\t' || src[9] == '\0');
}

IndDef*
parse_inductive(const char* src)
{
  Lexer lx;
  lexer_init(&lx, src);

  if (!expect(&lx, TOK_INDUCTIVE, "'inductive'"))
    return NULL;

  Token name_tok = lexer_consume(&lx);
  if (name_tok.kind != TOK_IDENT) {
    fprintf(stderr, "Parse error: expected type name after 'inductive'\n");
    return NULL;
  }

  int p_cap = 4, n_params = 0;
  Param* params = malloc(p_cap * sizeof(Param));

  while (lexer_peek(&lx).kind == TOK_LPAREN) {
    lexer_consume(&lx);
    Token pname = lexer_consume(&lx);
    if (pname.kind != TOK_IDENT) {
      fprintf(stderr, "Parse error: expected parameter name\n");
      free(params);
      return NULL;
    }
    if (!expect(&lx, TOK_COLON, "':'")) {
      free(params);
      return NULL;
    }
    Expr* pty = parse_expr(&lx);
    if (!pty) {
      free(params);
      return NULL;
    }
    if (!expect(&lx, TOK_RPAREN, "')'")) {
      free(params);
      return NULL;
    }

    if (n_params == p_cap) {
      p_cap *= 2;
      params = realloc(params, p_cap * sizeof(Param));
    }
    params[n_params].name = strdup(pname.text);
    params[n_params].type = pty;
    n_params++;
  }

  if (!expect(&lx, TOK_COLON, "':'")) {
    free(params);
    return NULL;
  }
  Expr* arity = parse_expr(&lx);
  if (!arity) {
    free(params);
    return NULL;
  }

  if (!expect(&lx, TOK_ASSIGN, "':='")) {
    free(params);
    return NULL;
  }

  int cap = 8, n_con = 0;
  ConDef* cons = malloc(cap * sizeof(ConDef));

  while (lexer_peek(&lx).kind == TOK_PIPE) {
    lexer_consume(&lx);
    Token con_tok = lexer_consume(&lx);
    if (con_tok.kind != TOK_IDENT) {
      fprintf(stderr, "Parse error: expected constructor name\n");
      free(cons);
      free(params);
      return NULL;
    }
    if (!expect(&lx, TOK_COLON, "':'")) {
      free(cons);
      free(params);
      return NULL;
    }
    Expr* con_type = parse_expr(&lx);
    if (!con_type) {
      free(cons);
      free(params);
      return NULL;
    }

    if (n_con == cap) {
      cap *= 2;
      cons = realloc(cons, cap * sizeof(ConDef));
    }
    cons[n_con].name = strdup(con_tok.text);
    cons[n_con].type = con_type;
    cons[n_con].arity = 0;
    cons[n_con].rec_args = NULL;
    cons[n_con].n_rec = 0;
    n_con++;
  }

  if (lexer_peek(&lx).kind == TOK_END)
    lexer_consume(&lx);

  if (n_con == 0) {
    fprintf(stderr, "Parse error: need at least one constructor\n");
    free(cons);
    free(params);
    return NULL;
  }

  return ind_register(
    strdup(name_tok.text), n_params, params, arity, n_con, cons);
}
