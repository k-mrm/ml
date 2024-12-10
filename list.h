#ifndef _ML_LIST_H
#define _ML_LIST_H

#include <stdio.h>
#include <stdlib.h>

typedef struct List List;

struct List {
  void *prev;
  void *next;
};

#define LIST(_ty) _ty *_prev, *_next
#define FOREACH(_head, _v)  \
  for (_v = (_head)->next; (List*)(_v) != _head; _v = (_v)->_next)
#define SAN(_head)  \
  do {  \
    if (!(_head)->prev || !(_head)->next) { \
      printf("bad list"); \
      exit(-1); \
    } \
  } while(0)

// LOL
#define FIRST(_head)  ((_head)->next)
#define SECOND(_head) (((List*)(FIRST(_head)))->next)
#define THIRD(_head)  (((List*)(SECOND(_head)))->next)
#define FOURTH(_head) (((List*)(THIRD(_head)))->next)
#define FIFTH(_head)  (((List*)(FOURTH(_head)))->next)
#define SIXTH(_head)  (((List*)(FIFTH(_head)))->next)
#define NEXT(_v)      ((_v)->next)
#define EMPTY(_head)  ((_head)->next == (_head))

#define PUSH(_head, _v) \
  do {  \
    SAN(_head);  \
    List *_p = (_head)->prev;  \
    (_head)->prev = (_v); \
    (_v)->_next = (void *)(_head);  \
    (_v)->_prev = (void *)_p; \
    _p->next = (_v);  \
  } while (0)

static inline int
length (List *head)
{
  struct dummy {
    LIST(struct dummy);
  } *dum;
  List *v;
  int i = 0;
  FOREACH (head, dum)
    i++;
  return i;
}

static void
delete (void *v)
{
  List *lv = (List *)v;
  List *next = lv->next;
  List *prev = lv->prev;
  next->prev = prev;
  prev->next = next;
}

static void
listinit(List *l)
{
  l->next = l;
  l->prev = l;
}

static List *
newlist (void)
{
  List *l = malloc (sizeof *l);
  listinit (l);
  return l;
}

#endif  // _ML_LIST_H
