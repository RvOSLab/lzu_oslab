#ifndef __SLEEP_H__
#define __SLEEP_H__
#include <stddef.h>
#include <utils/linked_list.h>

struct usleep_queue_node {
    int64_t remaining_time;
    struct linked_list_node list_node;
    struct task_struct *task;
};

void usleep_queue_init();
int64_t usleep_set(int64_t time);
void usleep_handler();

#endif
