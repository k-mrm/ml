#ifndef _ML_VALUE_H
#define _ML_VALUE_H

typedef struct Value    Value;

struct Value {
  const char *(*tostring)(Value *);

  union {
    struct {
      long long num;
    } n;
  };
};

Value *intvalue (long long n);

#endif  // _ML_VALUE_H
