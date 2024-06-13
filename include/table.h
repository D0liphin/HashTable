#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>

typedef char ctrl_t;

#define _CTRL_EMPTY 0
#define _CTRL_DEL 1
#define _CTRL_PRESENT -1

typedef intptr_t _key_t;
typedef intptr_t val_t;

struct entry
{
    ctrl_t meta;
    _key_t key;
    val_t val;
};

size_t hash(_key_t k)
{
    return (size_t)k;
}

typedef struct
{
    struct entry *buf;
    // How many entries are currently occupied?
    size_t nr_occupied;
    size_t capacity;
} table;

int table_init(table *, const size_t);

struct entry *table_get_slot(table *, _key_t);

int table_insert(table *, _key_t, val_t);

val_t *table_get(table *, _key_t);

void table_remove(table *, _key_t);

void table_free(table *);

int table_realloc(table *, size_t);

int table_init(table *tbl, const size_t capacity)
{
    tbl->buf = NULL;
    if (capacity > 0) {
        tbl->buf = (struct entry *)calloc(capacity, sizeof(struct entry));
        if (tbl->buf == NULL) {
            return ENOMEM;
        }
    }
    tbl->nr_occupied = 0;
    tbl->capacity = capacity;
    return 0;
}

struct entry *table_get_slot(table *tbl, _key_t k)
{
    size_t h = hash(k);
    for (size_t i = h % tbl->capacity; true; i = (i + 1) % tbl->capacity) {
        struct entry *e = &tbl->buf[i];
        if (e->meta == _CTRL_EMPTY) {
            return e;
        } else if (e->meta == _CTRL_DEL) {
            continue;
        } else if (e->key == k) {
            return e;
        }
    }
}

int table_insert(table *tbl, _key_t k, val_t v)
{
    if (tbl->capacity == 0 || tbl->nr_occupied >= tbl->capacity / 4 * 3) {
        int err = table_realloc(tbl, tbl->capacity ? tbl->capacity * 2 : 8);
        if (err) {
            return err;
        }
    }
    struct entry *e = table_get_slot(tbl, k);
    if (e->meta == _CTRL_EMPTY) {
        e->meta = _CTRL_PRESENT;
        e->key = k;
        e->val = v;
        tbl->nr_occupied++;
    } else {
        e->val = v;
    }
    return 0;
}

val_t *table_get(table *tbl, _key_t k)
{
    struct entry *e = table_get_slot(tbl, k);
    if (e->meta == _CTRL_EMPTY) {
        return NULL;
    } else {
        return &e->val;
    }
}

void table_remove(table *tbl, _key_t k)
{
    struct entry *e = table_get_slot(tbl, k);
    if (e->meta == _CTRL_PRESENT) {
        e->meta = _CTRL_DEL;
        tbl->nr_occupied--;
    }
}

void table_free(table *tbl)
{
    if (tbl->buf) {
        free(tbl->buf);
    }
    tbl->buf = NULL;
    tbl->capacity = 0;
    tbl->nr_occupied = 0;
}

int table_realloc(table *tbl, size_t new_capacity)
{
    table new_tbl;
    int err = table_init(&new_tbl, new_capacity);
    if (err) {
        return err;
    }
    for (size_t i = 0; i < tbl->capacity; ++i) {
        struct entry e = tbl->buf[i];
        if (e.meta == _CTRL_PRESENT) {
            table_insert(&new_tbl, e.key, e.val);
        }
    }
    table_free(tbl);
    *tbl = new_tbl;
    return 0;
}