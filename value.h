#ifndef _ML_VALUE_H
#define _ML_VALUE_H

#include <stdbool.h>
#include "ml.h"
#include "list.h"

typedef struct Value    Value;

struct Value {
  LIST (Value);
  const char *(*tostring) (Value *);
  bool (*eq) (Value *, Value *);

  union {
    struct {
      long long num;
    } n;
    struct {
      char *v;
      Env *env;
      Expr *e;
    } lam;
    struct {

    } ls;
  };
};

Value *intvalue (long long n);
Value *lamvalue (Env *env, char *id, Expr *e);

#endif  // _ML_VALUE_H
