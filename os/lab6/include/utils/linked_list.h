#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>

struct linked_list_node {
    struct linked_list_node *prev;
    struct linked_list_node *next;
};

static inline void linked_list_init(struct linked_list_node *head) {
    head->prev = head;
    head->next = head;
}

static inline void linked_list_insert_after(struct linked_list_node *target, struct linked_list_node *node) {
    node->next = target->next;
    node->prev = target;
    target->next = node;
    node->next->prev = node;
}

static inline void linked_list_insert_before(struct linked_list_node *target, struct linked_list_node *node) {
    node->prev = target->prev;
    node->next = target;
    target->prev = node;
    node->prev->next = node;
}

static inline void linked_list_remove(struct linked_list_node *target) {
    target->prev->next = target->next;
    target->next->prev = target->prev;
}

static inline uint64_t linked_list_empty(struct linked_list_node *head) {
    return head->next == head;
}

static inline struct linked_list_node *linked_list_first(struct linked_list_node *head) {
    if (linked_list_empty(head)) return (struct linked_list_node *)NULL;
    return head->next;
}

static inline struct linked_list_node *linked_list_last(struct linked_list_node *head) {
    if (linked_list_empty(head)) return (struct linked_list_node *)NULL;
    return head->prev;
}

static inline void linked_list_push(struct linked_list_node *head, struct linked_list_node *node) {
    linked_list_insert_before(head, node);
}

static inline struct linked_list_node *linked_list_pop(struct linked_list_node *head) {
    struct linked_list_node *last_node = linked_list_last(head);
    if (linked_list_empty(head)) return (struct linked_list_node *)NULL;
    linked_list_remove(last_node);
    return last_node;
}

static inline void linked_list_unshift(struct linked_list_node *head, struct linked_list_node *node) {
    linked_list_insert_after(head, node);
}

static inline struct linked_list_node *linked_list_shift(struct linked_list_node *head) {
    struct linked_list_node *first_node = linked_list_first(head);
    if (linked_list_empty(head)) return (struct linked_list_node *)NULL;
    linked_list_remove(first_node);
    return first_node;
}

#define for_each_linked_list_node(node, list_ptr) for (node = (list_ptr)->next; node != (list_ptr); node = node->next)

#endif
