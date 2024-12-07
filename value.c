#include <stdio.h>
#include <stdbool.h>
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

bool
inteq (Value *v1, Value *v2)
{
  return v1->n.num == v2->n.num;
}

Value *
intvalue (long long n)
{
  Value *v = malloc (sizeof *v);
  v->tostring = i2s;
  v->eq = inteq;
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

static bool
lameq (Value *v1, Value *v2)
{
  return v1 == v2;
}

Value *
lamvalue (Env *env, char *id, Expr *e)
{
  Value *v = malloc (sizeof *v);
  v->tostring = l2s;
  v->eq = lameq;
  v->lam.v = id;
  v->lam.e = e;
  v->lam.env = env;
  return v;
}
