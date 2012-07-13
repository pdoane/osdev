// ------------------------------------------------------------------------------------------------
// link.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "stdlib/types.h"

// ------------------------------------------------------------------------------------------------
typedef struct Link
{
    struct Link* prev;
    struct Link* next;
} Link;

// ------------------------------------------------------------------------------------------------
static inline void link_init(Link* x)
{
    x->prev = x;
    x->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void link_after(Link* a, Link* x)
{
    Link* p = a;
    Link* n = a->next;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void link_before(Link* a, Link* x)
{
    Link* p = a->prev;
    Link* n = a;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void link_remove(Link* a)
{
    Link* p = a->prev;
    Link* n = a->next;
    n->prev = p;
    p->next = n;
    a->next = 0;
    a->prev = 0;
}

// ------------------------------------------------------------------------------------------------
#define link_data(link,T,m) \
    (T*)((char*)(link) - (unsigned long)(&(((T*)0)->m)))

// ------------------------------------------------------------------------------------------------
#define list_for_each(it, list, m) \
    for (it = link_data((list).next, typeof(*it), m); \
        &it->m != &(list); \
        it = link_data(it->m.next, typeof(*it), m))

// ------------------------------------------------------------------------------------------------
#define list_for_each_safe(it, n, list, m) \
    for (it = link_data((list).next, typeof(*it), m), \
        n = link_data(it->m.next, typeof(*it), m); \
        &it->m != &(list); \
        it = n, \
        n = link_data(n->m.next, typeof(*it), m))
