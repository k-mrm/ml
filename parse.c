#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "ml.h"
#include "list.h"

/*
 *  CFG of ml:
 *   S ::= E ;
 *   E ::= L
 *   E ::= R
 *   E ::= A G
 *   E ::= M
 *   L ::= let id = E in E
 *   R ::= \ id -> E
 *   M ::= match E { I }
 *   I ::= E => E K
 *   K ::= ε
 *   K ::= | I
 *   G ::= ε
 *   G ::= . E
 *   A ::= T B
 *   B ::= ε
 *   B ::= + A
 *   B ::= - A
 *   T ::= F H
 *   H ::= ε
 *   H ::= * T
 *   H ::= / T
 *   F ::= id
 *   F ::= num
 *   F ::= ( E )
 */

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
  Token *c = (Token*)s->cur;
  s->cur = s->cur->next;
  return c;
}

typedef struct Parse  Parse;
typedef enum Sty      Sty;
typedef struct S      S;
typedef struct NT     NT;
typedef struct T      T;
typedef struct ParseTree ParseTree;

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

struct ParseTree {
  LIST(ParseTree); 
  Sty type;

  union {
    struct {
      Token *tk;
    } t;
    struct {
      char c;
      List *pt;
    } nt;
  };
};

static ParseTree *
tparsetree(Token *tk)
{
  ParseTree *pt = malloc(sizeof(*pt));
  pt->type = TERM;
  pt->t.tk = tk;
  return pt;
}

static ParseTree *
ntparsetree(char c, List *p)
{
  ParseTree *pt = malloc(sizeof(*pt));
  pt->type = NONTERM;
  pt->nt.c = c;
  pt->nt.pt = p;
  return pt;
}

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
  Expr *(*p2ast[128])(Parse *p, ParseTree *pt);
};

static void
parseerr(Parse *p, Token *tk)
{
  fprintf(stderr, "parse error at %s\n", tokenfmt(tk));
  exit(1);
}

//  S -> E;
static List *
slookup (Parse *p, Token *tk)
{
  List *r = malloc (sizeof(*r));
  listinit (r);

  switch (tk->tt) {
    case TOKEN_LET:
    case TOKEN_MATCH:
    case TOKEN_ID:
    case TOKEN_NAT:
    case TOKEN_LPAREN:
    case TOKEN_BS: {
      S *a = (S*)nt ('E');
      S *b = (S*)t (TOKEN_SEMI);
      PUSH (r, a);
      PUSH (r, b);
      return r;
    }
    default:
      parseerr (p, tk);
  }
}

//  E -> L | R | A G | M
static List *
elookup(Parse *p, Token *tk)
{
  List *r = malloc (sizeof(*r));
  listinit (r);

  switch (tk->tt) {
    case TOKEN_LET: {
      S *l = (S*)nt ('L');
      PUSH (r, l);
      return r;
    }
    case TOKEN_ID:
    case TOKEN_NAT:
    case TOKEN_LPAREN: {
      S *a = (S*)nt ('A');
      S *g = (S*)nt ('G');
      PUSH (r, a);
      PUSH (r, g);
      return r;
    }
    case TOKEN_BS: {
      S *lambda = (S*)nt ('R');
      PUSH (r, lambda);
      return r;
    }
    case TOKEN_MATCH: {
      S *m = (S*)nt ('M');
      PUSH (r, m);
      return r;
    }
    default:
      parseerr (p, tk);
  }
}

//  L -> let id = E in E
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

//  R -> \id -> E
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

//  M -> match E { I }
static List *
mlookup (Parse *p, Token *tk)
{
  List *r = malloc (sizeof (*r));
  listinit (r);

  switch (tk->tt) {
    case TOKEN_MATCH: {
      S *m = (S*)t (TOKEN_MATCH);
      S *e = (S*)nt ('E');
      S *lb = (S*)t (TOKEN_LBRACE);
      S *i = (S*)nt ('I');
      S *rb = (S*)t (TOKEN_RBRACE);
      PUSH (r, m);
      PUSH (r, e);
      PUSH (r, lb);
      PUSH (r, i);
      PUSH (r, rb);
      return r;
    }
    default:
      parseerr (p, tk);
  }
}

//  I -> E => E K
static List *
ilookup (Parse *p, Token *tk)
{
  List *r = malloc (sizeof(*r));
  listinit (r);

  switch (tk->tt) {
    case TOKEN_LET:
    case TOKEN_MATCH:
    case TOKEN_ID:
    case TOKEN_NAT:
    case TOKEN_LPAREN:
    case TOKEN_BS: {
      S *e1 = (S*)nt ('E');
      S *ar = (S*)t (TOKEN_FATARROW);
      S *e2 = (S*)nt ('E');
      S *k = (S*)nt ('K');
      PUSH (r, e1);
      PUSH (r, ar);
      PUSH (r, e2);
      PUSH (r, k);
      return r;
    }
    default:
      parseerr (p, tk);
  }
}

//  K -> eps | '|' I
static List *
klookup (Parse *p, Token *tk)
{
  List *r = malloc (sizeof(*r));
  listinit (r);

  switch (tk->tt) {
    case TOKEN_RBRACE:
      return r;
    case TOKEN_BAR:
      S *b = (S*)t (TOKEN_BAR);
      S *i = (S*)nt ('I');
      PUSH (r, b);
      PUSH (r, i);
      return r;
    default:
      parseerr (p, tk);
  }
}

//  T -> F H
static List *
tlookup(Parse *p, Token *tk)
{
  List *r = malloc (sizeof(*r));
  listinit (r);

  switch (tk->tt) {
    case TOKEN_ID:
    case TOKEN_NAT:
    case TOKEN_LPAREN: {
      S *f = (S*)nt ('F');
      S *h = (S*)nt ('H');
      PUSH (r, f);
      PUSH (r, h);
      return r;
    }
    default:
      parseerr (p, tk);
  }
}

//  H -> eps | *T | /T
static List *
hlookup(Parse *p, Token *tk)
{
  List *r = malloc(sizeof(*r));
  listinit(r);

  switch (tk->tt) {
    case TOKEN_ADD:
    case TOKEN_MINUS:
    case TOKEN_PERIOD:
    case TOKEN_RPAREN:
    case TOKEN_LBRACE:
    case TOKEN_RBRACE:
    case TOKEN_FATARROW:
    case TOKEN_BAR:
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

//  A -> T B
static List *
alookup (Parse *p, Token *tk)
{
  List *r = malloc (sizeof(*r));
  listinit (r);

  switch (tk->tt) {
    case TOKEN_ID:
    case TOKEN_NAT:
    case TOKEN_LPAREN: {
      S *t = (S*)nt ('T');
      S *b = (S*)nt ('B');
      PUSH (r, t);
      PUSH (r, b);
      return r;
    }
    default:
      parseerr (p, tk);
  }
}

//  B -> eps | +A | -A
static List *
blookup (Parse *p, Token *tk)
{
  List *r = malloc (sizeof(*r));
  listinit (r);

  switch (tk->tt) {
    case TOKEN_ADD: {
      S *a = (S*)t (TOKEN_ADD);
      S *e = (S*)nt ('A');
      PUSH(r, a);
      PUSH(r, e);
      return r;
    }
    case TOKEN_MINUS: {
      S *m = (S*)t (TOKEN_MINUS);
      S *e = (S*)nt ('A');
      PUSH(r, m);
      PUSH(r, e);
      return r;
    }
    case TOKEN_PERIOD:
    case TOKEN_RPAREN:
    case TOKEN_LBRACE:
    case TOKEN_RBRACE:
    case TOKEN_FATARROW:
    case TOKEN_BAR:
    case TOKEN_SEMI:
    case TOKEN_IN:
      return r;
    default:
      parseerr (p, tk);
  }
}

//  G -> eps | .E
static List *
glookup(Parse *p, Token *tk)
{
  List *r = malloc (sizeof(*r));
  listinit (r);

  switch (tk->tt) {
    case TOKEN_PERIOD: {
      S *d = (S*)t (TOKEN_PERIOD);
      S *e = (S*)nt ('E');
      PUSH (r, d);
      PUSH (r, e);
      return r;
    }
    case TOKEN_RPAREN:
    case TOKEN_LBRACE:
    case TOKEN_RBRACE:
    case TOKEN_SEMI:
    case TOKEN_FATARROW:
    case TOKEN_IN:
    case TOKEN_BAR:
      return r;
    default:
      parseerr (p, tk);
  }
}

//  F -> id | num | (E)
static List *
flookup (Parse *p, Token *tk)
{
  List *r = malloc (sizeof(*r));
  listinit (r);

  switch (tk->tt) {
    case TOKEN_ID: {
      S *id = (S*)t (TOKEN_ID);
      PUSH (r, id);
      return r;
    }
    case TOKEN_NAT: {
      S *nat = (S*)t (TOKEN_NAT);
      PUSH (r, nat);
      return r;
    }
    case TOKEN_LPAREN: {
      S *lp = (S*)t (TOKEN_LPAREN);
      S *e = (S*)nt ('E');
      S *rp = (S*)t (TOKEN_RPAREN);
      PUSH (r, lp);
      PUSH (r, e);
      PUSH (r, rp);
      return r;
    }
    default:
      parseerr (p, tk);
  }
}

static List *
undefined(Parse *p, Token *tk)
{
  panic("undefined");
}

static ParseTree *
parseexpr(Parse *p, S *s, int nest)
{
  if (s->type == TERM) {
    T *n = (T*)s;
    Token *tk = eat(&p->stream);
    if (tk->tt == n->tt) {
      for (int i = 0; i < nest*4; i++)
        trace(" ");
      trace("%s\n", tokenfmt(tk));
      return tparsetree(tk);
    } else {
      parseerr(p, tk);
    }
  } else if (s->type == NONTERM) {
    NT *n = (NT*)s;
    List *body = p->lut[n->c](p, lookahead(&p->stream));
    S *e;
    List *pts = malloc(sizeof(*pts));
    listinit(pts);
    for (int i = 0; i < nest*4; i++)
      trace(" ");
    trace("(%c\n", n->c);
    FOREACH (body, e) {
      ParseTree *pt = parseexpr(p, e, nest+1);
      PUSH(pts, pt);
    }
    for (int i = 0; i < nest*4; i++)
      trace(" ");
    trace(")\n");
    return ntparsetree(n->c, pts);
  } else {
    panic("unreachable");
  }
}

static Expr *ap2ast(ParseTree *pt);
static Expr *ep2ast(ParseTree *pt);
static Expr *tp2ast(ParseTree *pt);
static void ip2ast (ParseTree *pt, List *block);

/*
 *  S -> E;
 *  E -> L | R | A G | M
 *  L -> let id = E in E
 *  R -> \id -> E
 *  M -> match E { I }
 *  I -> E => E K
 *  K -> eps | '|' I
 *  G -> eps | .E
 *  A -> T B
 *  B -> eps | +A | -A
 *  T -> F H
 *  H -> eps | *T | /T
 *  F -> id | num | (E)
 */
static void
syntax(Parse *p)
{
  for (int i = 0; i < 128; i++)
    p->lut[i] = undefined;

  p->lut['S'] = slookup;
  p->lut['E'] = elookup;
  p->lut['L'] = letlookup;
  p->lut['R'] = lambdalookup;
  p->lut['M'] = mlookup;
  p->lut['I'] = ilookup;
  p->lut['K'] = klookup;
  p->lut['G'] = glookup;
  p->lut['A'] = alookup;
  p->lut['B'] = blookup;
  p->lut['T'] = tlookup;
  p->lut['H'] = hlookup;
  p->lut['F'] = flookup;
}

//  L -> let id = E in E
static Expr *
letast(ParseTree *pt)
{
  Expr *let = malloc(sizeof(*let));
  List *ptlist = pt->nt.pt;
  let->ty = E_LET;
  let->let.v = ((ParseTree*)SECOND(ptlist))->t.tk->ident;
  let->let.e = ep2ast(FOURTH(ptlist));
  let->let.ein = ep2ast(SIXTH(ptlist));
  return let;
}

//  R -> \id -> E
static Expr *
lambdaast(ParseTree *pt)
{
  Expr *lam = malloc(sizeof(*lam));
  List *ptlist = pt->nt.pt;
  lam->ty = E_LAM;
  lam->lam.v = ((ParseTree*)SECOND(ptlist))->t.tk->ident;
  lam->lam.body = ep2ast(FOURTH(ptlist));
  return lam;
}

//  K -> eps | '|' I
static void
kp2ast (ParseTree *pt, List *block)
{
  if (EMPTY (pt->nt.pt))
    return;

  ip2ast (SECOND (pt->nt.pt), block);
}

//  I -> E => E K
static void
ip2ast (ParseTree *pt, List *block)
{
  Expr *matblock = malloc (sizeof *matblock);
  matblock->ty = E_MATBLOCK;
  matblock->mb.match = ep2ast (FIRST (pt->nt.pt));
  matblock->mb.ret = ep2ast (THIRD (pt->nt.pt));
  PUSH (block, matblock);
  kp2ast (FOURTH (pt->nt.pt), block);
}

//  M -> match E { I }
static Expr *
mp2ast (ParseTree *pt)
{
  Expr *mat = malloc (sizeof (*mat));

  mat->ty = E_MAT;
  mat->mat.e = ep2ast (SECOND (pt->nt.pt));
  mat->mat.block = malloc (sizeof *mat->mat.block);
  listinit (mat->mat.block);
  ip2ast (FOURTH (pt->nt.pt), mat->mat.block);

  return mat;
}

static Expr *
binast(BinOp op, Expr *l, Expr *r)
{
  Expr *bin = malloc(sizeof(*bin));
  bin->ty = E_BIN;
  bin->b.op = op;
  bin->b.l = l;
  bin->b.r = r;
  return bin;
}

static Expr *
idast(char *v)
{
  Expr *e = malloc(sizeof(*e));
  e->ty = E_ID;
  e->id.v = v;
  return e;
}

static Expr *
natast(ulong nat)
{
  Expr *n = malloc(sizeof(*n));
  n->ty = E_NAT;
  n->n.nat = nat;
  return n;
}

//  G -> eps | .E
//  E -> L | R | A G | M
static Expr *
gp2ast (ParseTree *pt, Expr *e1)
{
  ParseTree *f;
  ParseTree *e;
  Expr *e2;
  if (EMPTY (pt->nt.pt))
    return e1;

  f = FIRST (pt->nt.pt);
  e = SECOND (pt->nt.pt);

  if (length (e->nt.pt) == 1) {
    e2 = ep2ast (e);
    switch (f->t.tk->tt) {
      case TOKEN_PERIOD:
        return binast (BIN_CALL, e1, e2);
      default:
        panic ("bug");
    }
  } else if (length (e->nt.pt) == 2) {
    e2 = ap2ast (FIRST (e->nt.pt));
    switch (f->t.tk->tt) {
      case TOKEN_PERIOD:
        return gp2ast (SECOND (e->nt.pt), binast (BIN_CALL, e1, e2));
      default:
        panic ("bug");
    }
  }
}

//  B -> eps | +A | -A
//  A -> T B
static Expr *
bp2ast (ParseTree *pt, Expr *e1)
{
  ParseTree *f;
  ParseTree *a;
  Expr *e2;
  if (EMPTY (pt->nt.pt))
    return e1;

  f = FIRST (pt->nt.pt);
  a = SECOND (pt->nt.pt);

  e2 = tp2ast (FIRST (a->nt.pt));
  switch (f->t.tk->tt) {
    case TOKEN_ADD:
      return bp2ast (SECOND (a->nt.pt), binast (BIN_ADD, e1, e2));
    case TOKEN_MINUS:
      return bp2ast (SECOND (a->nt.pt), binast (BIN_MINUS, e1, e2));
    default:
      panic("bug");
  }
}

//  F -> id | num | (E)
static Expr *
fp2ast(ParseTree *pt)
{
  ParseTree *f = FIRST(pt->nt.pt);
  switch (f->t.tk->tt) {
    case TOKEN_ID: return idast(f->t.tk->ident);
    case TOKEN_NAT: return natast(f->t.tk->nat);
    case TOKEN_LPAREN: return ep2ast(SECOND(pt->nt.pt));
    default: panic("bug");
  }
}

//  H -> eps | *T | /T
//  T -> F H
static Expr *
hp2ast(ParseTree *pt, Expr *e1)
{
  ParseTree *f;
  ParseTree *t;
  Expr *e2;
  if (EMPTY(pt->nt.pt))
    return e1;

  f = FIRST (pt->nt.pt);
  t = SECOND (pt->nt.pt);

  e2 = fp2ast (FIRST (t->nt.pt));
  switch (f->t.tk->tt) {
    case TOKEN_MUL:
      return hp2ast (SECOND (t->nt.pt), binast (BIN_MUL, e1, e2));
    case TOKEN_DIV:
      return hp2ast (SECOND (t->nt.pt), binast (BIN_DIV, e1, e2));
    default:
      panic("bug");
  }
}

//  T -> F H
static Expr *
tp2ast(ParseTree *pt)
{
  Expr *e1 = fp2ast(FIRST(pt->nt.pt));
  return hp2ast(SECOND(pt->nt.pt), e1);
}

//  A -> T B
static Expr *
ap2ast (ParseTree *pt)
{
  Expr *e1 = tp2ast(FIRST(pt->nt.pt));
  return bp2ast(SECOND(pt->nt.pt), e1);
}

//  E -> L | R | A G | M
static Expr *
ep2ast(ParseTree *pt)
{
  ParseTree *f = FIRST(pt->nt.pt);
  switch (f->nt.c) {
    case 'L':
      return letast(f);
    case 'R':
      return lambdaast(f);
    case 'A': {
      Expr *e1 = ap2ast(f);
      return gp2ast(SECOND(pt->nt.pt), e1);
    }
    case 'M':
      return mp2ast (f);
    default:
      panic("ep2ast");
  }
}

//  S -> E;
static Expr *
sp2ast(ParseTree *pt)
{
  List *ptlist = pt->nt.pt;
  return ep2ast(FIRST(ptlist));
}

void
exprdump(Expr *e, int nest)
{
  for (int i = 0; i < nest*2; i++)
    trace (" ");

  switch (e->ty) {
    case E_NAT: trace ("(%lu)\n", e->n.nat); break;
    case E_ID:  trace ("(%s)\n", e->id.v); break;
    case E_BIN:
      trace ("(%c \n", e->b.op);
      exprdump(e->b.l, nest+1);
      exprdump(e->b.r, nest+1);
      for (int i = 0; i < nest*2; i++)
        trace (" ");
      trace (")\n");
      break;
    case E_LET:
      trace ("(let %s = \n", e->let.v);
      exprdump(e->let.e, nest+1);
      for (int i = 0; i < nest*2; i++)
        trace (" ");
      trace ("in\n");
      exprdump(e->let.ein, nest+1);
      for (int i = 0; i < nest*2; i++)
        trace (" ");
      trace (")\n");
      break;
    case E_LAM:
      trace ("(\\%s -> \n", e->lam.v);
      exprdump(e->lam.body, nest+1);
      for (int i = 0; i < nest*2; i++)
        trace (" ");
      trace (")\n");
      break;
    case E_MAT: {
      Expr *b;
      trace ("match ");
      exprdump (e->mat.e, nest+1);
      FOREACH (e->mat.block, b) {
        exprdump (b, nest+1);
      }
      break;
    }
    case E_MATBLOCK:
      exprdump (e->mb.match, nest+1);
      for (int i = 0; i < nest*2; i++)
        trace (" ");
      trace (" => ");
      exprdump (e->mb.ret, nest+1);
      break;
  }
}

static List *
ast(List *ptlist)
{
  List *expr = malloc(sizeof(*expr));
  listinit(expr);
  ParseTree *pt;
  Expr *e;

  FOREACH (ptlist, pt) {
    e = sp2ast(pt);
    PUSH(expr, e);
  }

  FOREACH (expr, e) {
    exprdump(e, 0);
  }

  return expr;
}

List *
parse(List *tk)
{
  Parse *p = malloc(sizeof(*p));
  List *pts = malloc(sizeof(*pts));
  initstream(&p->stream, tk);
  listinit(pts);
  
  syntax(p);

  printf("\n");
  while (!eos(&p->stream)) {
    ParseTree *pt = parseexpr(p, (S*)nt('S'), 0);
    PUSH(pts, pt);
  }

  return ast(pts);
}
