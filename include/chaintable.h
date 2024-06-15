#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

size_t hashstr(char *str, size_t len)
{
    size_t h = 0;
    for (size_t i = 0; i < len; ++i) {
        char *hstr = (char *)&h;
        hstr[i % sizeof(size_t)] = str[i];
    }
    return h;
}

#define CHAINLINK_LEAF ((struct chainlink *)1)
#define CHAINLINK_EMPTY NULL

typedef intptr_t chaink_t;
typedef intptr_t chainv_t;

struct chainlink
{
    /* 
     * one of
     * - `CHAINLINK_LEAF`: this is the leaf node of this chain
     * - `CHAINLINK_EMPTY`: this chain is empty
     * or a pointer to the next element in this chain
     */
    struct chainlink *next;
    chaink_t key;
    chainv_t val;
};

typedef struct
{
    /*
     * a buffer in two parts. the first `cap` elements are for chain roots
     * the second part of the buffer acts as a space for allocating new 
     * nodes. `len` is used for both the load calculations, and for keeping
     * track of the initialized region of the second part.
     */
    struct chainlink *buf;
    /* the capacity of the buffer */
    size_t cap;
    /* the number of entries that have been inserted */
    size_t len;
    /* the load factor */
    double ldf;
} chaintable;

int chaintable_init(chaintable *, size_t, double);
int chaintable_insert(chaintable *, chaink_t, chainv_t);
chainv_t *chaintable_get(chaintable *, chaink_t);
void chaintable_remove(chaintable *, chaink_t);
void chaintable_free(chaintable *);
int chaintable_realloc(chaintable *, size_t);
double chaintable_load(const chaintable *);

/*
A simple chaining hashtable. 

TODO:
- We can potentially improve this table by using a union for chainlinks.
  Then, we can just mark nodes deleted and potentially reuse them. 
*/

static bool chainlink_is_empty(const struct chainlink link)
{
    return (intptr_t)(link.next) == (intptr_t)CHAINLINK_EMPTY;
}

static bool chainlink_is_leaf(const struct chainlink link)
{
    return (intptr_t)(link.next) == (intptr_t)CHAINLINK_LEAF;
}

int chaintable_init(chaintable *tbl, size_t cap, double ldf)
{
    tbl->buf = NULL;
    if (cap > 0) {
        tbl->buf = (struct chainlink *)calloc(cap * 2, sizeof(struct chainlink));
        if (tbl->buf == NULL) {
            return ENOMEM;
        }
    }
    tbl->cap = cap;
    tbl->ldf = ldf;
    tbl->len = 0;
    return 0;
}

int chaintable_insert(chaintable *tbl, chaink_t k, chainv_t v)
{
    if (chaintable_load(tbl) >= tbl->ldf) {
        int err = chaintable_realloc(tbl, tbl->cap * 2);
        if (err) {
            return err;
        }
    }
    size_t h = hashstr((char *)&k, sizeof(chaink_t));
    size_t i = h % tbl->cap;
    for (struct chainlink *link = tbl->buf + i;; link = link->next) {
        if (chainlink_is_empty(*link)) {
            *link = (struct chainlink){
                .next = CHAINLINK_LEAF,
                .key = k,
                .val = v,
            };
            tbl->len++;
            break;
        } else if (link->key == k) {
            link->val = v;
            break;
        } else if (chainlink_is_leaf(*link)) {
            link->next = tbl->buf + tbl->cap + tbl->len;
            *link->next = (struct chainlink){
                .next = CHAINLINK_LEAF,
                .key = k,
                .val = v,
            };
            tbl->len++;
            break;
        }
    }
    return 0;
}

chainv_t *chaintable_get(chaintable *tbl, chaink_t k)
{
    if (tbl->len == 0) {
        return NULL;
    }
    size_t h = hashstr((char *)&k, sizeof(chaink_t));
    size_t i = h % tbl->cap;
    for (struct chainlink *link = tbl->buf + i;; link = link->next) {
        if (chainlink_is_empty(*link)) {
            return NULL;
        } else if (link->key == k) {
            return &link->val;
        } else if (chainlink_is_leaf(*link)) {
            return NULL;
        }
    }
}

void chaintable_remove(chaintable *tbl, chaink_t k)
{
    if (tbl->len == 0) {
        return;
    }
    size_t h = hashstr((char *)&k, sizeof(chaink_t));
    size_t i = h % tbl->cap;

    struct chainlink *prevlink = tbl->buf + i, *link;
    if (chainlink_is_empty(*prevlink)) {
        return;
    }
    if (chainlink_is_leaf(*prevlink)) {
        prevlink->next = CHAINLINK_EMPTY;
        return;
    }
    link = prevlink->next;
    for (; !chainlink_is_leaf(*link); prevlink = link, link = link->next) {
        if (link->key == k) {
            if (chainlink_is_leaf(*link->next)) {
                prevlink->next = CHAINLINK_LEAF;
            } else {
                prevlink->next = link->next;
            }
            return;
        }
    }
}

void chaintable_free(chaintable *tbl)
{
    if (tbl->buf) {
        free(tbl->buf);
    }
    tbl->cap = 0;
    tbl->ldf = 0;
    tbl->len = 0;
}

int chaintable_realloc(chaintable *tbl, size_t cap)
{
    chaintable newtbl;
    chaintable_init(&newtbl, cap == 0 ? 8 : cap * 4, tbl->ldf);
    for (size_t i = 0; i < tbl->cap; ++i) {
        struct chainlink link = tbl->buf[i];
        if (chainlink_is_empty(link)) {
            continue;
        }
        for (;; link = *link.next) {
            chaintable_insert(&newtbl, link.key, link.val);
            if (chainlink_is_leaf(link)) {
                break;
            }
        }
    }
    chaintable_free(tbl);
    *tbl = newtbl;
    return 0;
}

double chaintable_load(const chaintable *tbl)
{
    return tbl->cap == 0 ? 1.0 : (double)tbl->len / (double)tbl->cap;
};