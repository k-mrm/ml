#ifndef _ML_ML_H
#define _ML_ML_H

#include "list.h"

#define NORETURN    __attribute__((noreturn))

typedef enum TokenType  TokenType;
typedef struct Token  Token;
typedef struct Expr   Expr;
typedef enum ExprType ExprType;
typedef enum BinOp    BinOp;
typedef struct Env    Env;

typedef unsigned char   uchar;
typedef unsigned int    uint;
typedef unsigned long   ulong;

// #define ML_DEBUG

#ifdef ML_DEBUG
#define trace(...)  printf (__VA_ARGS__)
#else
#define trace(...)  (void)(0)
#endif  // ML_DEBUG

enum TokenType {
  TOKEN_EOS,
  TOKEN_ID,
  TOKEN_NAT,
  TOKEN_REAL,

  TOKEN_ADD,    // +
  TOKEN_MINUS,  // -
  TOKEN_MUL,    // *
  TOKEN_DIV,    // /
  TOKEN_COMMA,  // ,
  TOKEN_PERIOD, // .
  TOKEN_SEMI,   // ;
  TOKEN_COLON,  // :
  TOKEN_ASSIGN, // =
  TOKEN_EQ,     // ==
  TOKEN_NEQ,	  // !=
  TOKEN_LT,	    // <
  TOKEN_GT,	    // >
  TOKEN_LTE,    // <=
  TOKEN_GTE,    // >=
  TOKEN_EX,     // !
  TOKEN_QUESTION, // ?
  TOKEN_BAR,    // |
  TOKEN_BS,     // `\`
  TOKEN_ARROW,  // ->
  TOKEN_FATARROW, // =>
  TOKEN_LPAREN, // (
  TOKEN_RPAREN, // )
  TOKEN_LBRACE, // {
  TOKEN_RBRACE, // }

  TOKEN_LET,
  TOKEN_MATCH,
  TOKEN_IN,
};

struct Token {
  LIST(Token);
  TokenType tt;
  union {
    ulong nat;
    double real;
    uchar *ident;
  };
  int line;
};

enum BinOp {
  BIN_ADD   = '+',
  BIN_MINUS = '-',
  BIN_MUL   = '*',
  BIN_DIV   = '/',
  BIN_CALL  = '.',
};

enum ExprType {
  E_NAT,
  E_ID,
  E_BIN,
  E_LET,
  E_LAM,
  E_MAT,
  E_MATBLOCK,
};

struct Expr {
  LIST(Expr);
  ExprType ty;

  union {
    struct {
      ulong nat;
    } n;
    struct {
      char *v; 
    } id;
    struct {
      BinOp op;
      Expr *l, *r;
    } b;
    struct {
      char *v;
      Expr *e, *ein;
    } let;
    struct {
      char *v;
      Expr *body;
    } lam;
    struct {
      Expr *e;
      List *block;    // mb list
    } mat;
    struct {
      Expr *match;
      Expr *ret;
    } mb;
  };
};

struct Env {
  Env *par;
  List *map;
};

void panic(char *s) NORETURN;
char *tokenfmt(Token *tk);
List *lex(const char *src);
List *parse(List *tk);
List *analyze(List *e);
int exec(List *e);

#endif	// _ML_ML_H
