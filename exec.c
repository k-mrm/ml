#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ml.h"
#include "list.h"
#include "value.h"

typedef struct Map    Map;

struct Map {
  LIST(Map);

  char *id;
  Value *v;
};

static Value *execexpr (Env *env, Expr *e);

static Value *
envlookup (Env *env, char *id)
{
  Env *e = env;
  Map *m;
  do {
    FOREACH (e->map, m) {
      if (strcmp (id, m->id) == 0)
        return m->v;
    }
    e = e->par;
  } while (e);
  return NULL;
}

static void
envreg (Env *env, char *id, Value *v)
{
  Map *m;
  FOREACH (env->map, m) {
    if (strcmp (id, m->id) == 0)
      goto rewrite;
  }

  m = malloc (sizeof *m);
  m->id = id;
  m->v = v;
  PUSH (env->map, m);
  return;

rewrite:
  m->v = v;
}

static Env *
emptyenv (Env *parent)
{
  Env *e = malloc (sizeof *e);
  e->par = parent;
  e->map = malloc (sizeof *e->map);
  listinit (e->map);
  return e;
}

static Value *
natexpr (Env *env, Expr *e)
{
  return intvalue (e->n.nat);
}

static Value *
idexpr (Env *env, Expr *e)
{
  return envlookup (env, e->id.v);
}

static Value *
vadd (Value *l, Value *r)
{
  return (Value*)intvalue (l->n.num + r->n.num);
}

static Value *
vsub (Value *l, Value *r)
{
  return (Value*)intvalue (l->n.num - r->n.num);
}

static Value *
vmul (Value *l, Value *r)
{
  return (Value*)intvalue (l->n.num * r->n.num);
}

static Value *
vdiv (Value *l, Value *r)
{
  return (Value*)intvalue (l->n.num / r->n.num);
}

static Value *
vcall (Value *l, Value *r)
{
  char *a = l->lam.v;
  Expr *body = l->lam.e;
  Env *env = l->lam.env;
  Env *new = emptyenv (env);
  envreg (new, a, r);
  return execexpr (new, body);
}

static Value *
binexpr (Env *env, Expr *e)
{
  Value *lv = execexpr (env, e->b.l);
  Value *rv = execexpr (env, e->b.r);

  switch (e->b.op) {
    case BIN_ADD: return vadd (lv, rv);
    case BIN_MINUS: return vsub (lv, rv);
    case BIN_MUL: return vmul (lv, rv);
    case BIN_DIV: return vdiv (lv, rv);
    case BIN_CALL: return vcall (lv, rv);
  }
}

static Value *
letexpr (Env *env, Expr *e)
{
  Env *new;
  Value *v = execexpr (env, e->let.e);
  envreg (env, e->let.v, v);
  new = emptyenv (env);
  return execexpr (new, e->let.ein);
}

static Value *
lamexpr (Env *env, Expr *e)
{
  return lamvalue (env, e->lam.v, e->lam.body);
}

static Value *
matexpr (Env *env, Expr *e)
{
  Expr *b;
  Value *mat = execexpr (env, e->mat.e);

  FOREACH (e->mat.block, b) {
    Value *c = execexpr (env, b->mb.match);

    if (!c || mat->eq (mat, c))
      return execexpr (env, b->mb.ret);
  }
  return NULL;
}

static Value *
execexpr (Env *env, Expr *e)
{
  switch (e->ty) {
    case E_NAT: return natexpr (env, e);
    case E_ID: return idexpr (env, e);
    case E_BIN: return binexpr (env, e);
    case E_LET: return letexpr (env, e);
    case E_LAM: return lamexpr (env, e);
    case E_MAT: return matexpr (env, e);
  }
  return NULL;
}

int
exec(List *elist)
{
  Expr *e;
  Env *env = emptyenv (NULL);
  Value *v;
  FOREACH (elist, e) {
    v = execexpr (env, e);
    printf ("%s\n", v->tostring (v));
  }
  return 0;
}
