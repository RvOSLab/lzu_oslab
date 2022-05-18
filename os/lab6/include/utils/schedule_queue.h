#ifndef SCHEDULE_QUEUE_H
#define SCHEDULE_QUEUE_H

#include <utils/linked_list.h>
#include <mm.h>
extern struct schedule_queue_node schedule_queue;

struct schedule_queue_node
{
    struct linked_list_node list_node;
    struct task_struct *task;
};

/**
 * 将某进程放入调度队尾
 */
static inline void push_process_to_schedule_queue(struct task_struct *p)
{
    struct schedule_queue_node *current_process_node = (struct schedule_queue_node *)kmalloc(sizeof(struct schedule_queue_node));
    current_process_node->task = p;
    linked_list_push(&schedule_queue.list_node, &current_process_node->list_node);
}

/**
 * 将某进程放入调度队头
 */
static inline void unshift_process_to_schedule_queue(struct task_struct *p)
{
    struct schedule_queue_node *current_process_node = (struct schedule_queue_node *)kmalloc(sizeof(struct schedule_queue_node));
    current_process_node->task = p;
    linked_list_unshift(&schedule_queue.list_node, &current_process_node->list_node);
}

// /**
//  * 将某进程放入调度队列倒数第二个位置
//  */
// static inline void insert_process_to_schedule_queue(struct task_struct *p)
// {
//     struct schedule_queue_node *current_process_node = (struct schedule_queue_node *)kmalloc(sizeof(struct schedule_queue_node));
//     current_process_node->task = p;
//     linked_list_insert_before(linked_list_last(&schedule_queue.list_node), &current_process_node->list_node);
// }

/**
 * 将某进程从调度队列中删除
 */
static inline void delete_process_from_schedule_queue(struct task_struct *p)
{
    struct linked_list_node *node;
    for_each_linked_list_node(node, &schedule_queue.list_node)
    {
        struct schedule_queue_node *cur_node = container_of(node, struct schedule_queue_node, list_node);
        if (cur_node->task == p)
        {
            linked_list_remove(node);
            kfree(cur_node);
            break;
        }
    }
}

/**
 * 弹出调度队列中第一个进程
 */
static inline struct task_struct *pop_first_process()
{
    struct linked_list_node *first_node = linked_list_shift(&schedule_queue.list_node); // 取出就绪队列第一个进程
    if (first_node)
    {
        struct schedule_queue_node *first_process_node = container_of(first_node, struct schedule_queue_node, list_node);
        struct task_struct *first_process = first_process_node->task;
        kfree(first_process_node);
        return first_process;
    }
    else
        return NULL;
}

static inline void schedule_queue_init()
{
    schedule_queue.task = NULL;
    linked_list_init(&schedule_queue.list_node);
}

#endif
