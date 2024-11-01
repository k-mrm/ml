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

#define PUSH(_head, _v) \
  do {  \
    SAN(_head);  \
    List *_p = (_head)->prev;  \
    (_head)->prev = (_v); \
    (_v)->_next = (void *)(_head);  \
    (_v)->_prev = (void *)_p; \
    _p->next = (_v);  \
  } while (0)

static void
listinit(List *l)
{
  l->next = l;
  l->prev = l;
}

#endif  // _ML_LIST_H
