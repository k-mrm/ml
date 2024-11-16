#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "list.h"
#include "ml.h"

typedef struct Dfa      Dfa;
typedef struct Lex      Lex;
typedef struct Delta    Delta;
typedef struct State    State;

struct State {
  int accept;
  Token *(*callback)(Lex *, uchar *);
  int id;
};

struct Delta {
  LIST(Delta);
  State *a;
  uchar in;
  State *b;
  bool fresh;
};

struct Dfa {
  State *q0;
  List delta;
};

struct Lex {
  const uchar *src;
  uchar *term;
  int cur;
  List tk;
  Dfa *dfa;
};

static uchar escaped[256] = {
  ['a'] = '\a',
  ['b'] = '\b',
  ['f'] = '\f',
  ['n'] = '\n',
  ['r'] = '\r',
  ['t'] = '\t',
  ['v'] = '\v',
  ['\\'] = '\\',
  ['\''] = '\'',
  ['\"'] = '\"',
  ['e'] = '\033',
  ['E'] = '\033'
};

static uchar *fmt[] = {
  [TOKEN_ADD]       = "+",
  [TOKEN_MINUS]     = "-", 
  [TOKEN_MUL]       = "*", 
  [TOKEN_DIV]       = "/", 
  [TOKEN_COMMA]     = ",",
  [TOKEN_PERIOD]    = ".",
  [TOKEN_COLON]     = ":",
  [TOKEN_SEMI]      = ";",
  [TOKEN_ASSIGN]    = "=",
  [TOKEN_EQ]        = "==",
  [TOKEN_NEQ]	      = "!=",
  [TOKEN_LT]	      = "<",
  [TOKEN_GT]	      = ">",
  [TOKEN_LTE]       = "<=",
  [TOKEN_GTE]       = ">=",
  [TOKEN_EX]        = "!",
  [TOKEN_QUESTION]  = "?",
  [TOKEN_BAR]       = "|",
  [TOKEN_BS]        = "\\",
  [TOKEN_ARROW]     = "->",
  [TOKEN_LPAREN]    = "(",
  [TOKEN_RPAREN]    = ")",
  [TOKEN_LET]       = "let",
  [TOKEN_MATCH]     = "match",
  [TOKEN_IN]        = "in",
  [TOKEN_EOS]       = "EOS",
};

char *
tokenfmt(Token *tk)
{
  switch (tk->tt) {
    case TOKEN_ID:
      return tk->ident;
    case TOKEN_NAT: {
      char *s = malloc(sizeof(char) * 64);
      sprintf(s, "%lu", tk->nat);
      return s;
    }
    default:
      return fmt[tk->tt];
  }
}

static void
tokendump(Token *tk)
{
  switch (tk->tt) {
    case TOKEN_EOS: printf("EOS"); return;
    case TOKEN_ID:  printf("ID(%s)", tk->ident); return;
    case TOKEN_NAT: printf("NAT(%ld)", tk->nat); return;
    default: printf("KW(%s)", fmt[tk->tt]); return;
  }
}

static Token *
newtoken(TokenType tt)
{
  Token *tk;

  tk = malloc(sizeof(*tk));
  if (!tk)
    return NULL;
  memset(tk, 0, sizeof(*tk));
  tk->tt = tt;

  return tk;
}

static void
dfastatedump(Dfa *m, State *q)
{
  Delta *d;
  FOREACH (&m->delta, d) {
    if (d->a == q)
      printf("q%d --'%c'--> q%d%s\n", q->id, d->in, d->b->id, d->b->accept ? "A" : "");
    if (d->b == q)
      printf("q%d --'%c'--> q%d%s\n", d->a->id, d->in, q->id, q->accept ? "A" : "");
  }
}

static void
dfastatedumpid(Dfa *m, int id)
{
  Delta *d;
  FOREACH (&m->delta, d) {
    if (d->a->id == id)
      printf("q%d --'%c'--> q%d%s\n", id, d->in, d->b->id, d->b->accept ? "A" : "");
    if (d->b->id == id)
      printf("q%d --'%c'--> q%d%s\n", d->a->id, d->in, d->b->id, d->b->accept ? "A" : "");
  }
}

static State *
state(int acc, Token *(*callback)(Lex *, uchar *))
{
  static int id = 0;
  State *q = malloc(sizeof(*q));
  if (!q)
    return NULL;

  memset(q, 0, sizeof *q);
  q->accept = acc;
  if (acc)
    q->callback = callback;
  q->id = id++;

  return q;
}

static Dfa *
newdfa(void)
{
  Dfa *dfa;

  dfa = malloc(sizeof(*dfa));
  if (!dfa)
    return NULL;
  memset(dfa, 0, sizeof(*dfa));
  listinit(&dfa->delta);
  dfa->q0 = state(0, NULL);

  return dfa;
}

static State *
__newdeltato(Dfa *dfa, State *from, uchar in, State *to, bool fresh)
{
  Delta *d;

  if (!to) {
    FOREACH (&dfa->delta, d) {
      if (d->a == from && d->in == in) {
        return d->b;
      }
    }
  }

  d = malloc(sizeof *d);
  if (!d)
    return NULL;
  if (!to) {
    to = state(0, NULL);
    if (!to)
      return NULL;
  }

  d->a = from;
  d->in = in;
  d->b = to;
  d->fresh = fresh;
  PUSH(&dfa->delta, d);

  return to;
}

static State *
newdeltato(Dfa *dfa, State *from, uchar in, State *to)
{
  return __newdeltato(dfa, from, in, to, false);
}

static State *
newdeltatofresh(Dfa *dfa, State *from, uchar in, State *to)
{
  return __newdeltato(dfa, from, in, to, true);
}

static State *
newdelta(Dfa *dfa, State *from, uchar in)
{
  return __newdeltato(dfa, from, in, NULL, false);
}

// --d-->q_target  --copy->  q_from --d--> q_to
static State *
deltacopy(Dfa *dfa, State *target, State *from, State *to)
{
  Delta *d;
  FOREACH (&dfa->delta, d) {
    if (!d->fresh && d->b == target) {
      newdeltatofresh(dfa, from, d->in, to);
    }
  }
  FOREACH (&dfa->delta, d)
    d->fresh = false;
  return to;
}

// q--d-->q_target  -->  q--d-->q_to
static State *
deltamove(Dfa *dfa, State *target, State *to)
{
  Delta *d;
  FOREACH (&dfa->delta, d) {
    if (d->b == target) {
      d->b = to;
    }
  }
  return to;
}

static uchar
nextch(Lex *l)
{
  return l->src[l->cur++];
}

static uchar
peek(Lex *l, int n)
{
  return l->src[l->cur + n];
}

static uchar
cur(Lex *l)
{
  return l->src[l->cur];
}

static int
lexend(Lex *l)
{
  return l->src[l->cur] == '\0';
}

static Token *
ident(Lex *l, uchar *ident)
{
  Token *tk;
  printf("ident is %s\n", ident);
  if (!strcmp(ident, "let"))
    return newtoken(TOKEN_LET);
  if (!strcmp(ident, "match"))
    return newtoken(TOKEN_MATCH);
  if (!strcmp(ident, "in"))
    return newtoken(TOKEN_IN);

  tk = newtoken(TOKEN_ID);
  tk->ident = malloc(sizeof(char) * (strlen(ident)+1));
  strcpy(tk->ident, ident);
  return tk;
}

static Token *
nat(Lex *l, uchar *n)
{
  Token *tk;
  tk = newtoken(TOKEN_NAT);
  printf("number is %s\n", n);
  tk->nat = atol(n);
  return tk;
}

static Token *
hex(Lex *l, uchar *n)
{
  Token *tk;
  tk = newtoken(TOKEN_NAT);
  printf("hex number is %s\n", n);
  // tk->ident = ident;
  return tk;
}

static Token *
blank(Lex *l, uchar *n)
{
  return NULL;
}

static Token *
symadd(Lex *l, uchar *_)
{
  return newtoken(TOKEN_ADD);
}

static Token *
symminus(Lex *l, uchar *_)
{
  return newtoken(TOKEN_MINUS);
}

static Token *
symmul(Lex *l, uchar *_)
{
  return newtoken(TOKEN_MUL);
}

static Token *
symdiv(Lex *l, uchar *_)
{
  return newtoken(TOKEN_DIV);
}

static Token *
symcolon(Lex *l, uchar *_)
{
  return newtoken(TOKEN_COLON);
}

static Token *
symsemi(Lex *l, uchar *_)
{
  return newtoken(TOKEN_SEMI);
}

static Token *
symex(Lex *l, uchar *_)
{
  return newtoken(TOKEN_EX);
}

static Token *
symquestion(Lex *l, uchar *_)
{
  return newtoken(TOKEN_QUESTION);
}

static Token *
symarrow(Lex *l, uchar *_)
{
  return newtoken(TOKEN_ARROW);
}

static Token *
symlparen(Lex *l, uchar *_)
{
  return newtoken(TOKEN_LPAREN);
}

static Token *
symrparen(Lex *l, uchar *_)
{
  return newtoken(TOKEN_RPAREN);
}

static Token *
symassign(Lex *l, uchar *_)
{
  return newtoken(TOKEN_ASSIGN);
}

static Token *
symeq(Lex *l, uchar *_)
{
  return newtoken(TOKEN_EQ);
}

static Token *
symneq(Lex *l, uchar *_)
{
  return newtoken(TOKEN_NEQ);
}

static void
defterm(Lex *l, uchar *pats)
{
  Dfa *m = l->dfa;
  State *q;
  l->term = pats;

  for (int i = 0; i < strlen(pats); i++) {
    q = newdelta(m, m->q0, pats[i]);
    q = newdeltato(m, q, pats[i], q);
    q->accept = 1;
    q->callback = blank;
  }
}

static void
defr(Lex *l, uchar *pat, Token *(*callback)(Lex *, uchar *))
{
  Dfa *m = l->dfa;
  State *q = m->q0;
  State *newq, *oldq;
  uchar c;

  for (int i = 0; i < strlen(pat); ) {
    c = pat[i++];
    if (c == '[') {
      oldq = q;
      newq = state(0, NULL);
      for (;;) {
        c = pat[i++];
        if (c == ']' || c == '\0')
          break;
        q = newdeltato(m, oldq, c, newq);
      }
    } else if (c == '*') {
      // r* (kleene star)
      if (!oldq)
        printf("regex error\n");
      q = deltamove(m, q, oldq);
    } else if (c == '+') {
      // r+
      q = deltacopy(m, q, q, q);
    } else if (c == '|') {
      q->accept = 1;
      q->callback = callback;
      if (!oldq)
        printf("regex error\n");
      q = oldq;
    } else {
      oldq = q;
      q = newdelta(m, q, c);
    }
  }

  q->accept = 1;
  q->callback = callback;
}

static void
def(Lex *l, uchar *pat, Token *(*callback)(Lex *, uchar *))
{
  Dfa *m = l->dfa;
  State *q = m->q0;

  for (int i = 0; i < strlen(pat); i++) {
    q = newdelta(m, q, pat[i]);
  }

  q->accept = 1;
  q->callback = callback;
}

static void
lexinit(Lex *l)
{
  l->dfa = newdfa();
  listinit(&l->tk);
  defterm(l, " \n\t");
  def(l, "->", symarrow);
  def(l, "+", symadd);
  def(l, "-", symminus);
  def(l, "*", symmul);
  def(l, "/", symdiv);
  def(l, "!", symex);
  def(l, "?", symquestion);
  def(l, ":", symcolon);
  def(l, ";", symsemi);
  def(l, "(", symlparen);
  def(l, ")", symrparen);
  def(l, "=", symassign);
  def(l, "==", symeq);
  def(l, "!=", symneq);
  defr(l, "[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_][abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_]*", ident);
  defr(l, "0x[0123456789abcdefABCDEF]+", hex);
  defr(l, "0|[123456789][0123456789]*", nat);
}

static State *
trans(Dfa *m, State *from, uchar in)
{
  Delta *d;
  FOREACH (&m->delta, d) {
    if (d->a == from && d->in == in) {
      return d->b;
    }
  }
  return NULL;
}

static bool
peekaccept(Dfa *m, State *from, uchar in)
{
  State *q = trans(m, from, in);
  return !!q;
}

static Token *
match(Lex *l)
{
  Dfa *m = l->dfa;
  State *q = m->q0;
  uchar c;
  uchar buf[128] = {0};
  int nbuf = 0;
  do {
    c = nextch(l);
    buf[nbuf++] = c;
    // printf("ch: '%c'\n", c);
    q = trans(m, q, c);
    // printf("q%p\n", q);
  } while (q && !q->accept || peekaccept(m, q, cur(l)));

  if (q && q->accept)
    return q->callback(l, buf);

  printf("unknown token %s\n", buf);
  exit(1);
}

static void
scan(Lex *l)
{
  Token *t;
  while (!lexend(l)) {
    if ((t = match(l)) != NULL) {
      PUSH(&l->tk, t);
    }
  }

  t = newtoken(TOKEN_EOS);
  PUSH(&l->tk, t);
}

List *
lex(const char *src)
{
  Lex *l = malloc(sizeof(*l));
  Token *tk;
  lexinit(l);
  l->src = src;
  l->cur = 0;
  scan(l);

  FOREACH (&l->tk, tk) {
    tokendump(tk);
  }

  return &l->tk;
}
