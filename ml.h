#ifndef _ML_ML_H
#define _ML_ML_H

#include "list.h"

typedef enum TokenType  TokenType;
typedef struct Token  Token;
typedef struct Expr   Expr;

typedef unsigned char   uchar;
typedef unsigned int    uint;
typedef unsigned long   ulong;

List *lex(const char *src);
Expr *parse(List *tk);
Expr *analyze(Expr *e);
int exec(Expr *e);

#endif	// _ML_ML_H
