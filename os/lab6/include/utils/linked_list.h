#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>

// 一个带头结点的双向循环链表，最后一个节点接到链表头，适用于放在结构体中，配合 container_of 函数使用
struct linked_list_node {
    struct linked_list_node *prev;
    struct linked_list_node *next;
};

#define INIT_LINKED_LIST(head)                                                 \
    { &(head), &(head) }

// 初始化头结点
static inline void linked_list_init(struct linked_list_node *head) {
    head->prev = head;
    head->next = head;
}

// 在某节点后插入新节点
static inline void linked_list_insert_after(struct linked_list_node *target, struct linked_list_node *node) {
    node->next = target->next;
    node->prev = target;
    target->next = node;
    node->next->prev = node;
}

// 在某节点前插入新节点
static inline void linked_list_insert_before(struct linked_list_node *target, struct linked_list_node *node) {
    node->prev = target->prev;
    node->next = target;
    target->prev = node;
    node->prev->next = node;
}

// 删除某节点，注意需要自行 free 包含链表的整个结构体
static inline void linked_list_remove(struct linked_list_node *target) {
    target->prev->next = target->next;
    target->next->prev = target->prev;
}

// 判断链表是否为空
static inline uint64_t linked_list_empty(struct linked_list_node *head) {
    return head->next == head;
}

// 返回链表第一个节点（除头结点外，若链表为空则返回 NULL）
static inline struct linked_list_node *linked_list_first(struct linked_list_node *head) {
    if (linked_list_empty(head)) return (struct linked_list_node *)NULL;
    return head->next;
}

// 返回链表最后一个节点（若链表为空则返回 NULL）
static inline struct linked_list_node *linked_list_last(struct linked_list_node *head) {
    if (linked_list_empty(head)) return (struct linked_list_node *)NULL;
    return head->prev;
}

// 将节点插入至链表尾
static inline void linked_list_push(struct linked_list_node *head, struct linked_list_node *node) {
    linked_list_insert_before(head, node);
}

// 返回链表尾节点并从删除
static inline struct linked_list_node *linked_list_pop(struct linked_list_node *head) {
    if (linked_list_empty(head)) return (struct linked_list_node *)NULL;
    struct linked_list_node *last_node = linked_list_last(head);
    linked_list_remove(last_node);
    return last_node;
}

// 将节点插入至链表第一个节点
static inline void linked_list_unshift(struct linked_list_node *head, struct linked_list_node *node) {
    linked_list_insert_after(head, node);
}

// 返回链表第一个节点并从删除
static inline struct linked_list_node *linked_list_shift(struct linked_list_node *head) {
    if (linked_list_empty(head)) return (struct linked_list_node *)NULL;
    struct linked_list_node *first_node = linked_list_first(head);
    linked_list_remove(first_node);
    return first_node;
}

// 遍历链表
#define for_each_linked_list_node(node, list_ptr) for (node = (list_ptr)->next; node != (list_ptr); node = node->next)

#endif
