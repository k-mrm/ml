#ifndef _ML_ML_H
#define _ML_ML_H

#include "list.h"

#define NORETURN    __attribute__((noreturn))

typedef enum TokenType  TokenType;
typedef struct Token  Token;
typedef struct Expr   Expr;

typedef unsigned char   uchar;
typedef unsigned int    uint;
typedef unsigned long   ulong;

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
  TOKEN_LPAREN, // (
  TOKEN_RPAREN, // )

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

void panic(char *s) NORETURN;
char *tokenfmt(Token *tk);
List *lex(const char *src);
List *parse(List *tk);
List *analyze(List *e);
int exec(List *e);

#endif	// _ML_ML_H
