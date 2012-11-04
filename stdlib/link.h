// ------------------------------------------------------------------------------------------------
// link.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
typedef struct Link
{
    struct Link *prev;
    struct Link *next;
} Link;

// ------------------------------------------------------------------------------------------------
static inline void LinkInit(Link *x)
{
    x->prev = x;
    x->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void LinkAfter(Link *a, Link *x)
{
    Link *p = a;
    Link *n = a->next;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void LinkBefore(Link *a, Link *x)
{
    Link *p = a->prev;
    Link *n = a;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void LinkRemove(Link *x)
{
    Link *p = x->prev;
    Link *n = x->next;
    n->prev = p;
    p->next = n;
    x->next = 0;
    x->prev = 0;
}

// ------------------------------------------------------------------------------------------------
static inline void LinkMoveAfter(Link *a, Link *x)
{
    Link *p = x->prev;
    Link *n = x->next;
    n->prev = p;
    p->next = n;

    p = a;
    n = a->next;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void LinkMoveBefore(Link *a, Link *x)
{
    Link *p = x->prev;
    Link *n = x->next;
    n->prev = p;
    p->next = n;

    p = a->prev;
    n = a;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline bool ListIsEmpty(Link *x)
{
    return x->next == x;
}

// ------------------------------------------------------------------------------------------------
#define LinkData(link,T,m) \
    (T *)((char *)(link) - (unsigned long)(&(((T*)0)->m)))

// ------------------------------------------------------------------------------------------------
#define ListForEach(it, list, m) \
    for (it = LinkData((list).next, typeof(*it), m); \
        &it->m != &(list); \
        it = LinkData(it->m.next, typeof(*it), m))

// ------------------------------------------------------------------------------------------------
#define ListForEachSafe(it, n, list, m) \
    for (it = LinkData((list).next, typeof(*it), m), \
        n = LinkData(it->m.next, typeof(*it), m); \
        &it->m != &(list); \
        it = n, \
        n = LinkData(n->m.next, typeof(*it), m))
