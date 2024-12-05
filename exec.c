#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ml.h"
#include "list.h"
#include "value.h"

typedef struct Env    Env;
typedef struct Map    Map;

struct Map {
  LIST(Map);

  char *id;
  Value *v;
};

struct Env {
  Env *par;
  List *map;
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
  Map *m = malloc (sizeof *m);
  m->id = id;
  m->v = v;
  PUSH (env->map, m);
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
  return (Value*)intvalue (e->n.nat);
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
binexpr (Env *env, Expr *e)
{
  Value *lv = execexpr (env, e->b.l);
  Value *rv = execexpr (env, e->b.r);

  switch (e->b.op) {
    case BIN_ADD: return vadd (lv, rv);
    case BIN_MINUS: return vsub (lv, rv);
    case BIN_MUL: return vmul (lv, rv);
    case BIN_DIV: return vdiv (lv, rv);
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
  ;
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
  }
  return NULL;
}

static void
exprdump(Expr *e, int nest)
{
  for (int i = 0; i < nest*2; i++)
    printf(" ");

  switch (e->ty) {
    case E_NAT: printf("(%lu)\n", e->n.nat); break;
    case E_ID:  printf("(%s)\n", e->id.v); break;
    case E_BIN:
      printf("(%c \n", e->b.op);
      exprdump(e->b.l, nest+1);
      exprdump(e->b.r, nest+1);
      for (int i = 0; i < nest*2; i++)
        printf(" ");
      printf(")\n");
      break;
    case E_LET:
      printf("(let %s = \n", e->let.v);
      exprdump(e->let.e, nest+1);
      for (int i = 0; i < nest*2; i++)
        printf(" ");
      printf("in\n");
      exprdump(e->let.ein, nest+1);
      for (int i = 0; i < nest*2; i++)
        printf(" ");
      printf(")\n");
      break;
    case E_LAM:
      printf("(\\%s -> \n", e->lam.v);
      exprdump(e->lam.body, nest+1);
      for (int i = 0; i < nest*2; i++)
        printf(" ");
      printf(")\n");
      break;
  }
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
}
