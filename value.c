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
