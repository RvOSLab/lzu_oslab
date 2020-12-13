#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stddef.h>
#include <sbi.h>
extern volatile size_t ticks;

void clock_init(void);
void clock_set_next_event(void);

#endif /* !__KERN_DRIVER_CLOCK_H__ */
