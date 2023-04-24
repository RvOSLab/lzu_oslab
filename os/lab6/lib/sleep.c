#ifdef COMPILE
#include <lib/sleep.h>
#include <stddef.h>
#include <sched.h>
#include <clock.h>
struct usleep_queue_node usleep_queue;

// 计时队列每次只对队首节点计时，时间到后移除队首节点，对新队首节点计时
void usleep_queue_init() {
    usleep_queue.remaining_time = 0;
    usleep_queue.task = NULL;
    linked_list_init(&usleep_queue.list_node);
}

int64_t usleep_set(int64_t utime)
{
    uint64_t is_disable = read_csr(sstatus) & SSTATUS_SIE;
    disable_interrupt();

    struct linked_list_node *node;
    for_each_linked_list_node(node, &usleep_queue.list_node)    // 找一个合适的队列插入位置
    {
        struct usleep_queue_node *cur_node = container_of(node, struct usleep_queue_node, list_node);
        utime -= cur_node->remaining_time;  // 计算上一个节点结束计时后当前节点还需计时多久
        if (utime <= 0) // 若剩余需计时时间小于等于 0，则表明找到合适插入位置，插入找到的节点之前
        {
            utime += cur_node->remaining_time;  // 并把多减去的剩余时间加回来
            break;
        }
    }
    struct usleep_queue_node new_node = {
        .remaining_time = utime,
        .task = current,
    };
    linked_list_insert_before(node, &new_node.list_node);   // 插入找到的节点之前或表尾
    container_of(node, struct usleep_queue_node, list_node)->remaining_time -= utime;   // 将插入节点的下一个节点计时时间减去差值

    sleep_on(&new_node.task);

    // 防止由其他事件唤醒进程导致计时队列没有删除，重复唤醒
    if (new_node.remaining_time > 0)
    {
        linked_list_remove(&new_node.list_node);
    }

    set_csr(sstatus, is_disable);
    return new_node.remaining_time;
}

void usleep_handler()
{
    while (!linked_list_empty(&usleep_queue.list_node)) {
        struct linked_list_node *first_list_node = linked_list_first(&usleep_queue.list_node);
        struct usleep_queue_node *first_usleep_node = container_of(first_list_node, struct usleep_queue_node, list_node);
        first_usleep_node->remaining_time -= 10000; // 对首节点减去一次时钟中断经过的时间
        if (first_usleep_node->remaining_time <= 0) {   // 若已到时
            linked_list_remove(first_list_node);    // 移除首节点
            // 不需要 free usleep_queue_node 结构体，结构体在那个被sleep的进程的内核栈上，唤醒返回值后直接被清除
            wake_up(&first_usleep_node->task);  // 唤醒 sleep 的进程
        } else {
            return;
        }
    }
}
#endif
