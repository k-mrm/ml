#ifndef _ML_VALUE_H
#define _ML_VALUE_H

#include "ml.h"

typedef struct Value    Value;

struct Value {
  const char *(*tostring)(Value *);

  union {
    struct {
      long long num;
    } n;
    struct {
      char *v;
      Env *env;
      Expr *e;
    } lam;
  };
};

Value *intvalue (long long n);
Value *lamvalue (Env *env, char *id, Expr *e);

#endif  // _ML_VALUE_H
