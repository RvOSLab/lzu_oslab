#ifndef LRU_H
#define LRU_H

#include "hash_table.h"
#include "linked_list.h"
#include <stddef.h>

struct lru_node {
    struct hash_table_node cache_node;
    struct linked_list_node used_list_node;
};

struct lru {
    uint64_t cache_length;
    uint64_t used_length;
    struct hash_table cache;
    struct linked_list_node used_list;
};

static inline void lru_init(struct lru *table) {
    hash_table_init(&(table->cache));
    linked_list_init(&(table->used_list));
    table->used_length = 0;
}

static inline struct lru_node *lru_get(struct lru *table, struct lru_node *node) {
    struct hash_table_node *hash_node = hash_table_get(&(table->cache), &(node->cache_node));
    if (!hash_node) return (struct lru_node *)NULL;
    struct lru_node *real_node = container_of(hash_node, struct lru_node, cache_node);
    linked_list_remove(&(real_node->used_list_node));
    linked_list_unshift(&(table->used_list), &(real_node->used_list_node));
    return real_node;
}

static inline struct lru_node *lru_set(struct lru *table, struct lru_node *node) {
    hash_table_set(&(table->cache), &(node->cache_node));
    linked_list_unshift(&(table->used_list), &(node->used_list_node));
    struct lru_node *dropped_node = (struct lru_node *)NULL;
    if (table->cache_length == table->used_length) {  // cache has full
        struct linked_list_node *dropped_list_node = linked_list_pop(&(table->used_list));
        dropped_node = container_of(dropped_list_node, struct lru_node, used_list_node);
        hash_table_del(&(table->cache), &(dropped_node->cache_node));
    } else {
        table->used_length += 1;
    }
    return dropped_node;
}

#endif
