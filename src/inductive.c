#define _POSIX_C_SOURCE 200809L
#include "inductive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "subst.h"

static IndDef* registry = NULL;

IndDef*
ind_lookup(const char* name)
{
  for (IndDef* d = registry; d; d = d->next)
    if (!strcmp(d->name, name))
      return d;
  return NULL;
}

IndDef*
ind_lookup_con(const char* con_name, int* idx)
{
  for (IndDef* d = registry; d; d = d->next)
    for (int i = 0; i < d->n_con; i++)
      if (!strcmp(d->cons[i].name, con_name)) {
        if (idx)
          *idx = i;
        return d;
      }
  return NULL;
}

IndDef*
ind_lookup_elim(const char* elim_name)
{
  for (IndDef* d = registry; d; d = d->next) {
    size_t tlen = strlen(d->name);
    if (strncmp(elim_name, d->name, tlen) == 0 &&
        strcmp(elim_name + tlen, "_elim") == 0)
      return d;
  }
  return NULL;
}

static int
is_recursive_arg(const char* ind_name, const Expr* ty)
{
  const Expr* cur = ty;
  while (cur->tag == E_APP)
    cur = cur->fun;
  return (cur->tag == E_VAR || cur->tag == E_IND) &&
         strcmp(cur->name, ind_name) == 0;
}

static int
collect_arg_types(const Expr* con_type, Expr*** arg_types_out)
{
  int cap = 8, n = 0;
  Expr** arr = malloc(cap * sizeof(Expr*));
  const Expr* cur = con_type;
  while (cur->tag == E_PI) {
    if (n == cap) {
      cap *= 2;
      arr = realloc(arr, cap * sizeof(Expr*));
    }
    arr[n++] = cur->type;
    cur = cur->body;
  }
  *arg_types_out = arr;
  return n;
}

static Expr*
build_app(Expr* f, Expr** args, int n)
{
  for (int i = 0; i < n; i++)
    f = e_app(f, expr_clone(args[i]));
  return f;
}

static Expr*
app_spine(const Expr* e, Expr*** args_out, int* n_args)
{
  int depth = 0;
  const Expr* cur = e;
  while (cur->tag == E_APP) {
    depth++;
    cur = cur->fun;
  }
  Expr** args = malloc((depth + 1) * sizeof(Expr*));
  int idx = depth - 1;
  cur = e;
  while (cur->tag == E_APP) {
    args[idx--] = cur->arg;
    cur = cur->fun;
  }
  *args_out = args;
  *n_args = depth;
  return (Expr*)cur;
}

static void
detect_indices(IndDef* d)
{
  d->is_index = calloc(d->n_params, sizeof(int));
  d->n_indices = 0;

  for (int p = 0; p < d->n_params; p++) {
    int is_idx = 0;
    for (int ci = 0; ci < d->n_con && !is_idx; ci++) {
      const Expr* cur = d->cons[ci].type;
      while (cur->tag == E_PI)
        cur = cur->body;
      Expr** ret_args = NULL;
      int n_ret = 0;
      app_spine(cur, &ret_args, &n_ret);
      if (p < n_ret) {
        const Expr* arg = ret_args[p];
        if (!(arg->tag == E_VAR && !strcmp(arg->name, d->params[p].name)))
          is_idx = 1;
      } else {
        is_idx = 1;
      }
      free(ret_args);
    }
    d->is_index[p] = is_idx;
    if (is_idx)
      d->n_indices++;
  }
}

static Expr*
ind_applied_full(IndDef* ind)
{
  Expr* t = e_ind(ind->name);
  for (int i = 0; i < ind->n_params; i++)
    t = e_app(t, e_var(ind->params[i].name));
  return t;
}

Expr*
ind_elim_type(IndDef* ind)
{
  Expr* T_full = ind_applied_full(ind);

  Expr* motive_ty = e_pi("_", expr_clone(T_full), e_star());
  for (int i = ind->n_params - 1; i >= 0; i--) {
    if (ind->is_index[i])
      motive_ty =
        e_pi(ind->params[i].name, expr_clone(ind->params[i].type), motive_ty);
  }

  Expr* P_applied = e_var("P");
  for (int i = 0; i < ind->n_params; i++)
    if (ind->is_index[i])
      P_applied = e_app(P_applied, e_var(ind->params[i].name));
  P_applied = e_app(P_applied, e_var("x"));

  Expr* result = e_pi("x", expr_clone(T_full), P_applied);

  for (int i = ind->n_params - 1; i >= 0; i--)
    if (ind->is_index[i])
      result =
        e_pi(ind->params[i].name, expr_clone(ind->params[i].type), result);

  for (int ci = ind->n_con - 1; ci >= 0; ci--) {
    ConDef* con = &ind->cons[ci];
    Expr** arg_tys = NULL;
    int n_args = collect_arg_types(con->type, &arg_tys);

    char** bnames = malloc(n_args * sizeof(char*));
    for (int j = 0; j < n_args; j++) {
      bnames[j] = malloc(32);
      snprintf(bnames[j], 32, "a%d", j);
    }

    const Expr* ret = con->type;
    char** orig_bnames = malloc(n_args * sizeof(char*));
    {
      const Expr* cur = con->type;
      for (int j = 0; j < n_args && cur->tag == E_PI; j++) {
        orig_bnames[j] = (char*)cur->binder;
        cur = cur->body;
      }
      ret = cur;
    }
    Expr** ret_args = NULL;
    int n_ret = 0;
    app_spine(ret, &ret_args, &n_ret);

    for (int r = 0; r < n_ret; r++) {
      for (int j = 0; j < n_args; j++) {
        if (orig_bnames[j] && strcmp(orig_bnames[j], bnames[j]) != 0) {
          Expr* substituted =
            subst(orig_bnames[j], e_var(bnames[j]), ret_args[r]);
          ret_args[r] = substituted;
        }
      }
    }

    for (int j2 = 0; j2 < n_args; j2++) {
      for (int j = 0; j < n_args; j++) {
        if (orig_bnames[j] && strcmp(orig_bnames[j], bnames[j]) != 0) {
          Expr* s = subst(orig_bnames[j], e_var(bnames[j]), arg_tys[j2]);
          arg_tys[j2] = s;
        }
      }
    }
    Expr* P_app = e_var("P");
    for (int i = 0; i < ind->n_params; i++)
      if (ind->is_index[i] && i < n_ret)
        P_app = e_app(P_app, expr_clone(ret_args[i]));

    Expr* con_app = e_con(con->name);
    for (int p = 0; p < ind->n_params; p++)
      if (!ind->is_index[p])
        con_app = e_app(con_app, e_var(ind->params[p].name));
    for (int j = 0; j < n_args; j++)
      con_app = e_app(con_app, e_var(bnames[j]));
    Expr* branch = e_app(P_app, con_app);

    for (int j = n_args - 1; j >= 0; j--) {
      if (is_recursive_arg(ind->name, arg_tys[j])) {
        char ih[32];
        snprintf(ih, sizeof(ih), "ih%d", j);
        const Expr* arg_ret = arg_tys[j];
        while (arg_ret->tag == E_PI)
          arg_ret = arg_ret->body;
        Expr** arg_ret_args = NULL;
        int n_arg_ret = 0;
        app_spine(arg_ret, &arg_ret_args, &n_arg_ret);

        Expr* ih_ty = e_var("P");
        for (int i = 0; i < ind->n_params; i++) {
          if (ind->is_index[i] && i < n_arg_ret) {
            Expr* idx_val = expr_clone(arg_ret_args[i]);
            for (int jj = 0; jj < n_args; jj++)
              if (orig_bnames[jj] && strcmp(orig_bnames[jj], bnames[jj]) != 0) {
                Expr* s = subst(orig_bnames[jj], e_var(bnames[jj]), idx_val);
                idx_val = s;
              }
            ih_ty = e_app(ih_ty, idx_val);
          }
        }
        ih_ty = e_app(ih_ty, e_var(bnames[j]));
        free(arg_ret_args);

        branch = e_pi(ih, ih_ty, branch);
      }
      branch = e_pi(bnames[j], expr_clone(arg_tys[j]), branch);
    }

    result = e_pi("_", branch, result);
    free(ret_args);
    free(orig_bnames);
    for (int j = 0; j < n_args; j++)
      free(bnames[j]);
    free(bnames);
    free(arg_tys);
  }

  result = e_pi("P", motive_ty, result);

  for (int i = ind->n_params - 1; i >= 0; i--)
    if (!ind->is_index[i])
      result =
        e_pi(ind->params[i].name, expr_clone(ind->params[i].type), result);

  return result;
}

Expr*
ind_iota(IndDef* ind, Expr** spine, int spine_len)
{
  int nu = ind->n_params - ind->n_indices;
  int ni = ind->n_indices;
  int expected = nu + 1 + ind->n_con + ni + 1;
  if (spine_len != expected)
    return NULL;

  int base = nu;

  Expr* scrut = whnf(spine[spine_len - 1]);
  Expr** all_args = NULL;
  int n_all = 0;
  Expr* head = app_spine(scrut, &all_args, &n_all);

  if (head->tag != E_CON && head->tag != E_VAR) {
    free(all_args);
    return NULL;
  }
  int ci = -1;
  IndDef* owner = ind_lookup_con(head->name, &ci);
  if (!owner || owner != ind) {
    free(all_args);
    return NULL;
  }

  ConDef* con = &ind->cons[ci];
  int n_val = n_all - nu;
  Expr** val = all_args + nu;

  Expr* branch = expr_clone(spine[base + 1 + ci]);

  int out_cap = con->arity + con->n_rec + 1;
  Expr** out = malloc(out_cap * sizeof(Expr*));
  int n_out = 0, ri = 0;

  for (int j = 0; j < con->arity && j < n_val; j++) {
    out[n_out++] = val[j];
    if (ri < con->n_rec && con->rec_args[ri] == j) {
      char elim_name[128];
      snprintf(elim_name, sizeof(elim_name), "%s_elim", ind->name);
      Expr* rec = e_elim(elim_name);
      for (int p = 0; p < ind->n_params; p++)
        if (!ind->is_index[p])
          rec = e_app(rec, expr_clone(spine[p < nu ? p : -1]));
      rec = e_elim(elim_name);
      int ui = 0;
      for (int p = 0; p < ind->n_params; p++)
        if (!ind->is_index[p])
          rec = e_app(rec, expr_clone(spine[ui++]));
      for (int k = base; k <= base + ind->n_con; k++)
        rec = e_app(rec, expr_clone(spine[k]));
      Expr** rec_arg_args = NULL;
      int n_rec_arg = 0;
      Expr* rec_head = app_spine(whnf(val[j]), &rec_arg_args, &n_rec_arg);
      (void)rec_head;
      for (int ix = 0; ix < ni && ix < n_rec_arg; ix++)
        rec = e_app(rec, expr_clone(rec_arg_args[ix]));
      free(rec_arg_args);
      rec = e_app(rec, expr_clone(val[j]));
      out[n_out++] = rec;
      ri++;
    }
  }

  Expr* result = build_app(branch, out, n_out);
  free(all_args);
  free(out);
  return result;
}

static Expr*
iota_hook_impl(const char* elim_name, Expr** args, int n)
{
  IndDef* ind = ind_lookup_elim(elim_name);
  if (!ind)
    return NULL;
  int nu = ind->n_params - ind->n_indices;
  int expected = nu + 1 + ind->n_con + ind->n_indices + 1;
  if (n != expected)
    return NULL;
  return ind_iota(ind, args, n);
}

void
ind_init(void)
{
  subst_set_iota_hook(iota_hook_impl);
}

IndDef*
ind_register(char* name,
             int n_params,
             Param* params,
             Expr* arity,
             int n_con,
             ConDef* cons)
{
  if (ind_lookup(name)) {
    fprintf(stderr, "Error: '%s' already defined\n", name);
    return NULL;
  }
  IndDef* d = calloc(1, sizeof(IndDef));
  d->name = name;
  d->n_params = n_params;
  d->params = params;
  d->arity = arity;
  d->n_con = n_con;
  d->cons = cons;

  for (int i = 0; i < n_con; i++) {
    ConDef* con = &cons[i];
    Expr** arg_tys = NULL;
    int n_args = collect_arg_types(con->type, &arg_tys);
    con->arity = n_args;
    int *rec = malloc((n_args + 1) * sizeof(int)), n_rec = 0;
    for (int j = 0; j < n_args; j++)
      if (is_recursive_arg(name, arg_tys[j]))
        rec[n_rec++] = j;
    con->rec_args = rec;
    con->n_rec = n_rec;
    free(arg_tys);
  }

  detect_indices(d);

  d->elim_type = ind_elim_type(d);
  d->next = registry;
  registry = d;

  for (int i = 0; i < n_con; i++)
    for (int p = n_params - 1; p >= 0; p--)
      if (!d->is_index[p])
        cons[i].type =
          e_pi(params[p].name, expr_clone(params[p].type), cons[i].type);
  return d;
}
