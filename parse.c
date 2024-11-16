#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ml.h"
#include "list.h"

typedef struct Stream Stream;

struct Stream {
  List *list;
  List *cur;
};

static void
initstream(Stream *s, List *l)
{
  s->list = l;
  s->cur = l->next;
}

static bool
eos(Stream *s)
{
  return ((Token*)s->cur)->tt == TOKEN_EOS;
}

static Token *
lookahead(Stream *s)
{
  return (Token*)s->cur;
}

Token *
eat(Stream *s)
{
  if (eos(s)) {
    return NULL;
  } else {
    Token *c = (Token*)s->cur;
    s->cur = s->cur->next;
    return c;
  }
}

typedef struct Parse  Parse;
typedef enum Sty      Sty;
typedef struct S      S;
typedef struct NT     NT;
typedef struct T      T;

enum Sty {
  TERM,
  NONTERM,
};

struct S {
  LIST(S);
  Sty type;
};

struct NT {
  struct S s;
  char c;
};

struct T {
  struct S s;
  TokenType tt;
};

static NT *
nt(char c)
{
  NT *n = malloc(sizeof *n);
  ((S*)n)->type = NONTERM;
  n->c = c;
  return n;
}

static T *
t(TokenType tt)
{
  T *n = malloc(sizeof *n);
  ((S*)n)->type = TERM;
  n->tt = tt;
  return n;
}

struct Parse {
  Stream stream;
  List expr;
  List *(*lut[128])(Parse *p, Token *tk);
};

struct Expr {
  LIST(Expr);

  union {

  };
};

static void
parseerr(Parse *p, Token *tk)
{
  fprintf(stderr, "parse error at %s\n", tokenfmt(tk));
  exit(1);
}

static List *
slookup(Parse *p, Token *tk)
{
  List *r = malloc(sizeof(*r));
  listinit(r);

  switch (tk->tt) {
    case TOKEN_LET:
    case TOKEN_MATCH:
    case TOKEN_ID:
    case TOKEN_NAT:
    case TOKEN_LPAREN:
    case TOKEN_BS: {
      S *a = (S*)nt('E');
      S *b = (S*)t(TOKEN_SEMI);
      PUSH(r, a);
      PUSH(r, b);
      return r;
    }
    default:
      parseerr(p, tk);
  }
}

static List *
elookup(Parse *p, Token *tk)
{
  List *r = malloc(sizeof(*r));
  listinit(r);

  switch (tk->tt) {
    case TOKEN_LET: {
      S *l = (S*)nt('L');
      PUSH(r, l);
      return r;
    }
    case TOKEN_MATCH:
      parseerr(p, tk);  // unimpl
    case TOKEN_ID:
    case TOKEN_NAT:
    case TOKEN_LPAREN: {
      S *tt = (S*)nt('T');
      S *g = (S*)nt('G');
      PUSH(r, tt);
      PUSH(r, g);
      return r;
    }
    case TOKEN_BS: {
      S *lambda = (S*)nt('R');
      PUSH(r, lambda);
      return r;
    }
    default:
      parseerr(p, tk);
  }
}

static List *
letlookup(Parse *p, Token *tk)
{
  List *r = malloc(sizeof(*r));
  listinit(r);

  switch (tk->tt) {
    case TOKEN_LET: {
      S *l = (S*)t(TOKEN_LET);
      S *id = (S*)t(TOKEN_ID);
      S *as = (S*)t(TOKEN_ASSIGN);
      S *e1 = (S*)nt('E');
      S *in = (S*)t(TOKEN_IN);
      S *e2 = (S*)nt('E');
      PUSH(r, l);
      PUSH(r, id);
      PUSH(r, as);
      PUSH(r, e1);
      PUSH(r, in);
      PUSH(r, e2);
      return r;
    }
    default:
      parseerr(p, tk);
  }
}

static List *
lambdalookup(Parse *p, Token *tk)
{
  List *r = malloc(sizeof(*r));
  listinit(r);

  switch (tk->tt) {
    case TOKEN_BS: {
      S *bs = (S*)t(TOKEN_BS);
      S *id = (S*)t(TOKEN_ID);
      S *ar = (S*)t(TOKEN_ARROW);
      S *e = (S*)nt('E');
      PUSH(r, bs);
      PUSH(r, id);
      PUSH(r, ar);
      PUSH(r, e);
      return r;
    }
    default:
      parseerr(p, tk);
  }
}

static List *
tlookup(Parse *p, Token *tk)
{
  List *r = malloc(sizeof(*r));
  listinit(r);

  switch (tk->tt) {
    case TOKEN_ID:
    case TOKEN_NAT:
    case TOKEN_LPAREN: {
      S *f = (S*)nt('F');
      S *h = (S*)nt('H');
      PUSH(r, f);
      PUSH(r, h);
      return r;
    }
    default:
      parseerr(p, tk);
  }
}

static List *
hlookup(Parse *p, Token *tk)
{
  List *r = malloc(sizeof(*r));
  listinit(r);

  switch (tk->tt) {
    case TOKEN_ADD:
    case TOKEN_MINUS:
    case TOKEN_RPAREN:
    case TOKEN_SEMI:
    case TOKEN_IN:
      return r;
    case TOKEN_MUL: {
      S *m = (S*)t(TOKEN_MUL);
      S *tt = (S*)nt('T');
      PUSH(r, m);
      PUSH(r, tt);
      return r;
    }
    case TOKEN_DIV: {
      S *d = (S*)t(TOKEN_DIV);
      S *tt = (S*)nt('T');
      PUSH(r, d);
      PUSH(r, tt);
      return r;
    }
    default:
      parseerr(p, tk);
  }
}

static List *
glookup(Parse *p, Token *tk)
{
  List *r = malloc(sizeof(*r));
  listinit(r);

  switch (tk->tt) {
    case TOKEN_ADD: {
      S *a = (S*)t(TOKEN_ADD);
      S *e = (S*)nt('E');
      PUSH(r, a);
      PUSH(r, e);
      return r;
    }
    case TOKEN_MINUS: {
      S *m = (S*)t(TOKEN_MINUS);
      S *e = (S*)nt('E');
      PUSH(r, m);
      PUSH(r, e);
      return r;
    }
    case TOKEN_RPAREN:
    case TOKEN_SEMI:
    case TOKEN_IN:
      return r;
    default:
      parseerr(p, tk);
  }
}

static List *
flookup(Parse *p, Token *tk)
{
  List *r = malloc(sizeof(*r));
  listinit(r);

  switch (tk->tt) {
    case TOKEN_ID: {
      S *id = (S*)t(TOKEN_ID);
      PUSH(r, id);
      return r;
    }
    case TOKEN_NAT: {
      S *nat = (S*)t(TOKEN_NAT);
      PUSH(r, nat);
      return r;
    }
    case TOKEN_LPAREN: {
      S *lp = (S*)t(TOKEN_LPAREN);
      S *e = (S*)nt('E');
      S *rp = (S*)t(TOKEN_RPAREN);
      PUSH(r, lp);
      PUSH(r, e);
      PUSH(r, rp);
      return r;
    }
    default:
      parseerr(p, tk);
  }
}

static List *
undefined(Parse *p, Token *tk)
{
  panic("undefined");
}

static Expr *
parseexpr(Parse *p, S *s, int nest)
{
  if (s->type == TERM) {
    T *n = (T*)s;
    Token *tk = eat(&p->stream);
    if (tk->tt == n->tt) {
      for (int i = 0; i < nest*4; i++)
        printf(" ");
      printf("%s\n", tokenfmt(tk));
      // return parsetree(tk);
      return NULL;
    } else {
      parseerr(p, tk);
    }
  } else if (s->type == NONTERM) {
    NT *n = (NT*)s;
    List *body = p->lut[n->c](p, lookahead(&p->stream));
    S *e;
    for (int i = 0; i < nest*4; i++)
      printf(" ");
    printf("(%c\n", n->c);
    FOREACH (body, e) {
      parseexpr(p, e, nest+1);
    }
    for (int i = 0; i < nest*4; i++)
      printf(" ");
    printf(")\n");
    return NULL;
  } else {
    panic("unreachable");
  }
}

static void
syntax(Parse *p)
{
  for (int i = 0; i < 128; i++)
    p->lut[i] = undefined;

  p->lut['S'] = slookup;
  p->lut['E'] = elookup;
  p->lut['L'] = letlookup;
  p->lut['R'] = lambdalookup;
  p->lut['T'] = tlookup;
  p->lut['H'] = hlookup;
  p->lut['G'] = glookup;
  p->lut['F'] = flookup;
}

List *
parse(List *tk)
{
  Parse *p = malloc(sizeof(*p));
  if (!p)
    return NULL;
  initstream(&p->stream, tk);
  listinit(&p->expr);
  
  syntax(p);

  printf("\n");
  while (!eos(&p->stream)) {
    Expr *e = parseexpr(p, (S*)nt('S'), 0);
    // PUSH(&p->expr, e);
  }

  return &p->expr;
}
