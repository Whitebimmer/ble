#ifndef SYS_TIMER_H
#define SYS_TIMER_H


#include "typedef.h"
#include "list.h"





struct sys_timer{
	struct list_head entry;
	void (*fun)(struct sys_timer *);
	u32 jiffies;
    u32 user;
};














#endif

