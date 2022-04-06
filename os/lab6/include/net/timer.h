#ifndef TIMER_H
#define TIMER_H

#include <net/net_utils.h>
#include <net/list.h>


struct timer {
	struct      list_head list;
	int         refcnt;					        /* 引用计数 */
	uint32_t    expires;		                /* 过期时间 */
	int         cancelled;				        /* 是否需要取消 */
	void        (*handler)(uint32_t, void *);   /* 处理函数 */
	void        *arg;						    /* 调用参数 */
};

struct timer *timer_add(uint32_t expire, void(*handler)(uint32_t, void*), void *arg);
void timer_release(struct timer *t);
void timer_cancel(struct timer *t);
void *timers_start();
int timer_get_tick();

void init_timer_lock();


#endif // !TIMER_H
