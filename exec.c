#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ml.h"
#include "list.h"
#include "value.h"

typedef struct Map    Map;
typedef struct String String;

struct Map {
  LIST (Map);

  char *id;
  Expr *expr;
};

struct String {
  LIST (String);

  char *str;
};

static Value *execexpr (Env *env, Expr *e);
void exprdump(Expr *e, int nest);
static Expr *envlookup (Env *env, char *id);

String *
string (char *s)
{
  String *a = malloc (sizeof (*a));
  a->str = s;
  return a;
}

static bool
isfreev (List *l, char *v)
{
  String *s;
  FOREACH (l, s) {
    if (strcmp (s->str, v) == 0) {
      return true;
    }
  }
  return false;
}

static void
delfreev (List *l, char *del)
{
  String *s;
  FOREACH (l, s) {
    if (strcmp (s->str, del) == 0) {
      delete (s);
    }
  }
}

static List *
freevar (Expr *e)
{
  String *s;
  List *l = malloc (sizeof *l);
  listinit (l);

  switch (e->ty) {
    case E_NAT:
      return l;
    case E_ID:
      s = string (e->id.v);
      PUSH (l, s);
      return l;
    case E_BIN: {
      List *l1 = freevar (e->b.l);
      List *l2 = freevar (e->b.r);
      FOREACH (l1, s) {
        String *s1 = string (s->str);
        PUSH (l, s1);
      }
      FOREACH (l2, s) {
        String *s1 = string (s->str);
        PUSH (l, s1);
      }

      free (l1);
      free (l2);
      return l;
    }
    case E_LET: {
      List *l1 = freevar (e->let.e);
      List *l2 = freevar (e->let.ein);
      FOREACH (l1, s) {
        String *s1 = string (s->str);
        PUSH (l, s1);
      }
      FOREACH (l2, s) {
        String *s1 = string (s->str);
        PUSH (l, s1);
      }
      delfreev (l, e->let.v);

      free (l1);
      free (l2);
      return l;
    }
    case E_LAM: {
      List *l = freevar (e->lam.body);
      delfreev (l, e->lam.v);
      return l;
    }
    case E_MAT: {
      List *l1 = freevar (e->mat.e);
      Expr *mb;

      FOREACH (l1, s) {
        String *s1 = string (s->str);
        PUSH (l, s1);
      }

      FOREACH (e->mat.block, mb) {
        List *lm = freevar (mb->mb.match);
        List *lr = freevar (mb->mb.ret);
        FOREACH (lm, s) {
          String *s1 = string (s->str);
          PUSH (l, s1);
        }
        FOREACH (lr, s) {
          String *s1 = string (s->str);
          PUSH (l, s1);
        }

        free (lm);
        free (lr);
      }
      free (l1);

      return l;
    }
    case E_CFN:
      return l;
  }
  return NULL;
}

static char *
newname (void)
{
  static unsigned long long i = 0;
  char *n = malloc(16);
  sprintf (n, "_ML%lld", i++);
  return n;
}

static void
changename (Expr *expr, Env *env, char *old, char *new)
{
  switch (expr->ty) {
    case E_NAT: return;
    case E_ID:
      if (strcmp (expr->id.v, old) == 0) {
        expr->id.v = new;
      }
      return;
    case E_BIN:
      changename (expr->b.l, env, old, new);
      changename (expr->b.r, env, old, new);
      return;
    case E_MAT: {
      Expr *mb;
      changename (expr->mat.e, env, old, new);
      trace ("change name to %s -> %s\n", old, new);
      FOREACH (expr->mat.block, mb) {
        changename (mb->mb.match, env, old, new);
        changename (mb->mb.ret, env, old, new);
      }
      return;
    }
    case E_LET:
      changename (expr->let.e, env, old, new);
      changename (expr->let.ein, env, old, new);
      return;
    case E_LAM:
      changename (expr->lam.body, env, old, new);
      return;
    case E_CFN:
      return;
  }
}

static void
__alphaconv (Env *env, Expr *expr, List *fv)
{
  switch (expr->ty) {
    case E_NAT: return;
    case E_ID:
      return;
    case E_LET:
      if (isfreev (fv, expr->let.v)) {
        char *a = newname ();
        changename (expr->let.e, env, expr->let.v, a);
        changename (expr->let.ein, env, expr->let.v, a);
        expr->let.v = a;
      }
      __alphaconv (env, expr->let.e, fv);
      __alphaconv (env, expr->let.ein, fv);
      return;
    case E_LAM:
      if (isfreev (fv, expr->lam.v)) {
        char *a = newname ();
        changename (expr->lam.body, env, expr->lam.v, a);
        expr->lam.v = a;
      }
      __alphaconv (env, expr->lam.body, fv);
      return;
    case E_BIN:
      __alphaconv (env, expr->b.l, fv);
      __alphaconv (env, expr->b.r, fv);
      return;
    case E_MAT: {
      Expr *mb;
      __alphaconv (env, expr->mat.e, fv);
      FOREACH (expr->mat.block, mb) {
        __alphaconv (env, mb->mb.match, fv);
        __alphaconv (env, mb->mb.ret, fv);
      }
      return;
    }
    case E_CFN:
      return;
  }
}

static void
alphaconv (Env *env, Expr *expr, Expr *assign)
{
  List *fv = freevar (assign);
  String *s;

  exprdump (assign, 0);
  FOREACH (fv, s) {
    trace ("%s is freevar\n", s->str);
  }

  if (length (fv) > 0) {
    exprdump (expr, 0);

    __alphaconv (env, expr, fv);

    trace ("alphaconv --> ");
    exprdump (expr, 0);
  }

  free (fv);
}

static Expr *
copy (Expr *e)
{
  Expr *e2 = malloc (sizeof *e2);
  *e2 = *e;
  e2->_prev = e2->_next = NULL;

  switch (e->ty) {
    case E_NAT: 
    case E_ID:
    case E_CFN:
      return e2;
    case E_BIN:
      e2->b.l = copy (e->b.l);
      e2->b.r = copy (e->b.r);
      return e2;
    case E_LET:
      e2->let.e = copy (e->let.e);
      e2->let.ein = copy (e->let.ein);
      return e2;
    case E_LAM:
      e2->lam.body = copy (e->lam.body);
      return e2;
    case E_MAT: {
      Expr *mb, *newmb;
      e2->mat.e = copy (e->mat.e);
      e2->mat.block = newlist ();
      FOREACH (e->mat.block, mb) {
        newmb = copy (mb);
        PUSH (e2->mat.block, newmb);
      }
      return e2;
    }
    case E_MATBLOCK:
      e2->mb.match = copy (e->mb.match);
      e2->mb.ret = copy (e->mb.ret);
      return e2;
    default:
      panic ("what");
  }
}

static void
__betar (Expr *e, char *v, Expr *to)
{
  switch (e->ty) {
    case E_NAT:
    case E_CFN: return;
    case E_ID:
      if (strcmp (e->id.v, v) == 0)
        *e = *to;
      return;
    case E_BIN:
      __betar (e->b.l, v, to);
      __betar (e->b.r, v, to);
      return;
    case E_LET:
      if (strcmp (e->let.v, v) != 0) {
        __betar (e->let.e, v, to);
        __betar (e->let.ein, v, to);
      }
      return;
    case E_LAM:
      if (strcmp (e->lam.v, v) != 0)
        __betar (e->lam.body, v, to);
      return;
    case E_MAT: {
      Expr *mb;
      __betar (e->mat.e, v, to);
      FOREACH (e->mat.block, mb) {
        __betar (mb->mb.match, v, to);
        __betar (mb->mb.ret, v, to);
      }
      return;
    }
  }
}

static Expr *
betareduction (Expr *e, char *v, Expr *to)
{
  trace ("beta reduction %s -> ", v);
  exprdump (e, 0);
  Expr *newe = copy (e);
  __betar (newe, v, to);
  trace ("beta reduction done!!!!!!!==============>  ");
  exprdump (newe, 0);
  return newe;
}

static Expr *
envlookup (Env *env, char *id)
{
  Env *e = env;
  Map *m;
  do {
    FOREACH (e->map, m) {
      if (strcmp (id, m->id) == 0)
        return m->expr;
    }
    e = e->par;
  } while (e);
  return NULL;
}

static void
envreg (Env *env, char *id, Expr *e)
{
  Map *m;
  FOREACH (env->map, m) {
    if (strcmp (id, m->id) == 0)
      goto rewrite;
  }

  m = malloc (sizeof *m);
  m->id = id;
  m->expr = e;
  PUSH (env->map, m);
  return;

rewrite:
  m->expr = e;
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

static Expr *
cfnexpr (Expr *(*cfn) (Env *))
{
  Expr *e = malloc (sizeof *e);
  e->ty = E_CFN;
  e->c.cfn = cfn;
  return e; 
}

static Expr *cstdout (void);

// stdout = \__stdout_arg -> printf.__stdout_arg
static Expr *
__stdout (Env *env)
{
  Value *v;
  Expr *e = envlookup (env, "__stdout_arg");
  if (!e)
    panic ("bug");

  v = execexpr (env, e);
  printf ("%s ", v->tostring (v));

  return cstdout ();
}

static Expr *
cstdout (void)
{
  Expr *lam = malloc (sizeof *lam);
  lam->ty = E_LAM;
  lam->lam.v = "__stdout_arg";
  lam->lam.body = cfnexpr (__stdout);
  return lam;
}

static Value *
natexpr (Env *env, Expr *e)
{
  return intvalue (e->n.nat);
}

static Value *
idexpr (Env *env, Expr *e)
{
  Expr *expr = envlookup (env, e->id.v);

  if (expr)
    return execexpr (env, expr);
  else
    return NULL;
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
vcall (Env *env, Expr *le, Expr *re)
{
  alphaconv (env, le, re);
  Value *l = execexpr (env, le);
  char *a = l->lam.v;
  Expr *body = l->lam.e;
  Env *lamenv = l->lam.env;
  Env *new = emptyenv (lamenv);
  envreg (new, a, re);
  trace ("envreg %s <-", a);
  exprdump (re, 0);
  return execexpr (new, betareduction (body, a, re));
}

static Value *
binexpr (Env *env, Expr *e)
{
  switch (e->b.op) {
    case BIN_ADD: return vadd (execexpr (env, e->b.l), execexpr (env, e->b.r));
    case BIN_MINUS: return vsub (execexpr (env, e->b.l), execexpr (env, e->b.r));
    case BIN_MUL: return vmul (execexpr (env, e->b.l), execexpr (env, e->b.r));
    case BIN_DIV: return vdiv (execexpr (env, e->b.l), execexpr (env, e->b.r));
    case BIN_CALL: return vcall (env, e->b.l, e->b.r);
  }
}

static Value *
letexpr (Env *env, Expr *e)
{
  Env *new;

  alphaconv (env, e->let.ein, e->let.e);
  envreg (env, e->let.v, e->let.e);
  new = emptyenv (env);
  return execexpr (new, betareduction (e->let.ein, e->let.v, e->let.e));
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
execcfn (Env *env, Expr *e)
{
  return execexpr (env, e->c.cfn (env));
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
    case E_CFN: return execcfn (env, e);
  }
  return NULL;
}

static void
embedcfn (Env *env)
{
  envreg (env, "stdout", cstdout ());
}

int
exec(List *elist)
{
  Expr *e;
  Env *env = emptyenv (NULL);
  Value *v;

  embedcfn (env);

  FOREACH (elist, e) {
    v = execexpr (env, e);
    trace ("(trace) %s\n", v->tostring (v));
  }
  return 0;
}
