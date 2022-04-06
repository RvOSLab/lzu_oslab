#include <net/timer.h>
#include <net/socket.h>
#include <clock.h>
#include <utils/atomic.h>
#include <kdebug.h>
#include <mm.h>
#include <lib/sleep.h>

static LIST_HEAD(timers);
struct spinlock lock;
static uint64_t tick = 0;


void init_timer_lock()  {  
    init_lock(&lock, "timer_lock");  
}

static void timer_free(struct timer *t) {
	if (lock.locked != 0) { // 如果锁被占用了,也就是暂时还不能释放
		kprintf("Timer free mutex lock");
		return;
	}
    acquire_lock(&lock);
	list_del(&t->list);
	kfree(t);
    release_lock(&lock);
}

static struct timer *timer_alloc() {
	struct timer *t = kmalloc(sizeof(struct timer));
	return t;
}

static void timers_tick() {
    // kprintf("timer_tick");
	struct list_head *item, *tmp = NULL;
	struct timer *t = NULL;

    list_for_each_safe(item, tmp, &timers) {	/* 遍历每个timer(定时器),如果
												到期,则执行,过期或者取消,则释放.*/
        t = list_entry(item, struct timer, list);

        if (!t->cancelled && t->expires < tick) {
            t->cancelled = 1;
            t->handler(tick, t->arg);
        }

        if (t->cancelled && t->refcnt == 0) {
            timer_free(t);
        }
    }
}

struct timer *timer_add(uint32_t expire, void (*handler)(uint32_t, void*), void *arg) {
	struct timer *t = timer_alloc();
	t->refcnt = 1;
	t->expires = tick + expire;
	t->cancelled = 0;
	// 这种现象应该出现得不多吧.
	if (t->expires < tick) {
		kprintf("ERR: Timer expiry integer wrap aroud\n");
	}

	t->handler = handler;
	t->arg = arg;
	acquire_lock(&lock);
	// 因为要对list进行操作,所以要加锁
	// 插入的顺序不要紧
	list_add_tail(&t->list, &timers); // 将t添加到timers的后面
	release_lock(&lock);
	return t;
}

void timer_release(struct timer *t) {
	if (lock.locked != 0) {
		kprintf("Timer release lock");
		return;
	}
    acquire_lock(&lock);
	if (t) {
		t->refcnt--;
	}
    release_lock(&lock);
}

void timer_cancel(struct timer *t) {
	// 一旦一个timer被取消,那么在时钟滴答的过程中,这个timer将会被删除
	if (lock.locked != 0) {
		kprintf("Timer cancel lock");
		return;
	}
    acquire_lock(&lock);
	if (t) {
		t->refcnt--;
		t->cancelled = 1;
	}
	release_lock(&lock);
}

void *timers_start() {
	while (1) {
        usleep_set(10000);
		tick++;
		timers_tick();
	}
}

int timer_get_tick() {
	return tick;
}


