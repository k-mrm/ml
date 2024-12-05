#include <stdio.h>
#include "ml.h"
#include "list.h"
#include "value.h"

static const char *
i2s (Value *v)
{
  char *s = malloc (64);
  sprintf (s, "%lld", v->n.num);
  return s;
}

Value *
intvalue (long long n)
{
  Value *v = malloc (sizeof *v);
  v->tostring = i2s;
  v->n.num = n;
  return v;
}

static const char *
l2s (Value *v)
{
  char *s = malloc (64);
  sprintf (s, "lambda \%s -> %p", v->lam.v, v->lam.e);
  return s;
}

Value *
lamvalue (Env *env, char *id, Expr *e)
{
  Value *v = malloc (sizeof *v);
  v->tostring = l2s;
  v->lam.v = id;
  v->lam.e = e;
  v->lam.env = env;
  return v;
}
